#include "InvoiceRecognizer.h"
#include "../core/OcrManager.h"
#include "../classifiers/InvoiceClassifier.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QStringList>
#include <QtConcurrent>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <functional>
#include <algorithm>

namespace {
QFile* debugLogFile = nullptr;

void debugLog(const QString &message)
{
    qDebug() << message;

    if (!debugLogFile) {
        debugLogFile = new QFile("invoice_debug.log");
        if (debugLogFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
            QTextStream stream(debugLogFile);
            stream << "\n\n========== NEW SESSION " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " ==========\n";
        }
    }

    if (debugLogFile && debugLogFile->isOpen()) {
        QTextStream stream(debugLogFile);
        stream << QDateTime::currentDateTime().toString("hh:mm:ss") << " " << message << "\n";
        stream.flush();
    }
}
}

namespace {
QJsonObject parseJsonObjectFromText(QString text)
{
    text = text.trimmed();
    if (text.startsWith("```")) {
        int firstNewline = text.indexOf('\n');
        if (firstNewline >= 0) {
            text = text.mid(firstNewline + 1);
        }
        if (text.endsWith("```")) {
            text.chop(3);
        }
        text = text.trimmed();
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8(), &err);
    if (!doc.isNull() && doc.isObject()) {
        return doc.object();
    }

    const int objStart = text.indexOf('{');
    const int objEnd = text.lastIndexOf('}');
    if (objStart >= 0 && objEnd > objStart) {
        const QString jsonPart = text.mid(objStart, objEnd - objStart + 1);
        doc = QJsonDocument::fromJson(jsonPart.toUtf8(), &err);
        if (!doc.isNull() && doc.isObject()) {
            return doc.object();
        }
    }
    return QJsonObject();
}

QString normalizeKey(QString key)
{
    key = key.trimmed().toLower();
    key.remove(QRegularExpression(QStringLiteral(R"([\s_\-:：()（）\[\]【】])")));
    return key;
}

bool isMeaningfulString(const QString &value)
{
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }
    return trimmed.compare(QStringLiteral("null"), Qt::CaseInsensitive) != 0
        && trimmed.compare(QStringLiteral("none"), Qt::CaseInsensitive) != 0
        && trimmed != QStringLiteral("-")
        && trimmed != QStringLiteral("--");
}

QString firstNonEmpty(const QJsonObject &json, const QStringList &keys)
{
    for (const QString &key : keys) {
        const QString value = json.value(key).toString().trimmed();
        if (isMeaningfulString(value)) {
            return value;
        }
    }
    return QString();
}

QJsonValue findValueByKeysDeep(const QJsonValue &root, const QStringList &keys)
{
    const QStringList normalizedTargets = [&keys]() {
        QStringList result;
        for (const QString &k : keys) {
            const QString nk = normalizeKey(k);
            if (!nk.isEmpty()) {
                result.append(nk);
            }
        }
        return result;
    }();

    // First pass: exact match only
    std::function<QJsonValue(const QJsonValue &, bool)> walk = [&](const QJsonValue &node, bool exactOnly) -> QJsonValue {
        if (node.isObject()) {
            const QJsonObject obj = node.toObject();

            // First check for exact matches
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                const QString keyNorm = normalizeKey(it.key());
                for (const QString &target : normalizedTargets) {
                    if (keyNorm == target) {
                        if (it.value().isDouble()) {
                            return it.value();
                        }
                        if (it.value().isString() && isMeaningfulString(it.value().toString())) {
                            return it.value();
                        }
                    }
                }
            }

            // If no exact match and not exact-only, try substring match
            if (!exactOnly) {
                for (auto it = obj.begin(); it != obj.end(); ++it) {
                    const QString keyNorm = normalizeKey(it.key());
                    bool match = false;
                    for (const QString &target : normalizedTargets) {
                        // Only allow longer target to contain keyNorm (e.g., "amount" matches "totalAmount")
                        // but not the other way around (e.g., "rate" should not match "taxRate")
                        if (keyNorm.length() >= target.length() && keyNorm.contains(target)) {
                            match = true;
                            break;
                        }
                    }

                    if (match) {
                        if (it.value().isDouble()) {
                            return it.value();
                        }
                        if (it.value().isString() && isMeaningfulString(it.value().toString())) {
                            return it.value();
                        }
                    }
                }
            }

            // Recurse into nested objects/arrays
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                if (it.value().isObject() || it.value().isArray()) {
                    QJsonValue nested = walk(it.value(), exactOnly);
                    if (!nested.isUndefined()) {
                        return nested;
                    }
                }
            }
        } else if (node.isArray()) {
            const QJsonArray array = node.toArray();
            for (const QJsonValue &item : array) {
                if (item.isObject() || item.isArray()) {
                    QJsonValue nested = walk(item, exactOnly);
                    if (!nested.isUndefined()) {
                        return nested;
                    }
                }
            }
        }
        return QJsonValue(QJsonValue::Undefined);
    };

    // Try exact match first
    QJsonValue result = walk(root, true);
    if (!result.isUndefined()) {
        return result;
    }

    // Fall back to substring match
    return walk(root, false);
}

QString extractRawText(const QJsonObject &json)
{
    QStringList chunks;

    auto appendFromKey = [&](const QString &key) {
        if (!json.contains(key)) {
            return;
        }
        const QJsonValue val = json.value(key);
        if (val.isString()) {
            const QString text = val.toString().trimmed();
            if (!text.isEmpty()) {
                chunks << text;
            }
        } else if (val.isArray()) {
            const QJsonArray arr = val.toArray();
            for (const QJsonValue &v : arr) {
                if (v.isObject()) {
                    const QJsonObject obj = v.toObject();
                    const QString t = obj.value(QStringLiteral("text")).toString().trimmed();
                    if (!t.isEmpty()) {
                        chunks << t;
                    }
                } else if (v.isString()) {
                    const QString t = v.toString().trimmed();
                    if (!t.isEmpty()) {
                        chunks << t;
                    }
                }
            }
        }
    };

    appendFromKey(QStringLiteral("text"));
    appendFromKey(QStringLiteral("raw_result"));
    appendFromKey(QStringLiteral("rawText"));
    appendFromKey(QStringLiteral("content"));
    appendFromKey(QStringLiteral("lines"));
    appendFromKey(QStringLiteral("boxes"));

    if (chunks.isEmpty() && json.contains(QStringLiteral("data")) && json.value(QStringLiteral("data")).isObject()) {
        chunks << extractRawText(json.value(QStringLiteral("data")).toObject());
    }

    return chunks.join('\n').trimmed();
}

struct AmountCandidate {
    double value = 0.0;
    int score = 0;
    QString context;
};

QList<double> extractNumbers(const QString &text)
{
    QList<double> numbers;
    if (text.isEmpty()) {
        return numbers;
    }

    QRegularExpression re(QStringLiteral(R"([-+]?\d{1,3}(?:,\d{3})*(?:\.\d+)?|[-+]?\d+(?:\.\d+)?)"));
    auto it = re.globalMatch(text);
    while (it.hasNext()) {
        const QString token = it.next().captured(0);
        bool ok = false;
        const double value = QString(token).remove(',').toDouble(&ok);
        if (ok) {
            numbers.append(value);
        }
    }
    return numbers;
}

// Candidate scoring for amount extraction - avoids picking tax rate/invoice number
double extractLabeledAmount(const QString &rawText, const QStringList &positiveLabels, const QStringList &negativeLabels = {})
{
    if (rawText.isEmpty() || positiveLabels.isEmpty()) {
        return 0.0;
    }

    debugLog(QString("extractLabeledAmount called with positive labels: %1").arg(positiveLabels.join(", ")));

    const QStringList lines = rawText.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);
    QList<AmountCandidate> candidates;

    // Negative keywords that indicate non-amount numbers
    const QStringList defaultNegative = {
        QStringLiteral("税率"), QStringLiteral("%"), QStringLiteral("百分比"),
        QStringLiteral("编号"), QStringLiteral("代码"), QStringLiteral("号码"),
        QStringLiteral("纳税人识别号"), QStringLiteral("统一社会信用代码"),
        QStringLiteral("身份证"), QStringLiteral("数量"), QStringLiteral("单价"), QStringLiteral("件"),
        QStringLiteral("序号"), QStringLiteral("电话"), QStringLiteral("手机")
    };
    const QStringList &negLabels = negativeLabels.isEmpty() ? defaultNegative : negativeLabels;

    // Number regex for extracting amounts
    QRegularExpression numRe(QStringLiteral(R"(([￥¥$]\s*)?(\d+(?:,\d{3})*(?:\.\d+)?)(?!\.?\d))"));
    if (!numRe.isValid()) {
        debugLog(QString("  Regex error: %1").arg(numRe.errorString()));
    }

    // Helper lambda to extract numbers from a line
    auto extractNumbersFromLine = [&](const QString &line, int baseScore, const QString &contextLabel) -> QList<AmountCandidate> {
        QList<AmountCandidate> lineCandidates;
        QRegularExpressionMatchIterator it = numRe.globalMatch(line);
        int matchCount = 0;
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            matchCount++;
            debugLog(QString("  Regex match #%1: captured(0)='%2', captured(1)='%3', captured(2)='%4'")
                .arg(matchCount)
                .arg(match.captured(0))
                .arg(match.captured(1))
                .arg(match.captured(2)));

            QString numStr = match.captured(2);
            bool ok = false;
            const double value = numStr.remove(',').toDouble(&ok);
            if (!ok || value <= 0) {
                continue;
            }

            AmountCandidate candidate;
            candidate.value = value;
            candidate.context = line;
            candidate.score = baseScore;

            // Check if number is near currency symbol or keywords
            const int numPos = match.capturedStart(2);
            const QString beforeNum = line.left(numPos);
            const QString afterNum = line.mid(numPos + numStr.length());

            // Positive: currency symbols nearby
            if (beforeNum.contains(QChar(0xFFE5)) || beforeNum.contains(QChar(0x00A5)) ||
                beforeNum.contains('$') || beforeNum.contains(QStringLiteral("¥")) ||
                beforeNum.contains(QStringLiteral("元"))) {
                candidate.score += 15;
            }
            if (afterNum.contains(QChar(0xFFE5)) || afterNum.contains(QChar(0x00A5)) ||
                afterNum.contains(QStringLiteral("元"))) {
                candidate.score += 10;
            }

            // Negative: percent sign nearby (tax rate)
            if (beforeNum.contains('%') || afterNum.contains('%') ||
                beforeNum.contains(QStringLiteral("％")) || afterNum.contains(QStringLiteral("％"))) {
                candidate.score -= 20;
            }

            // Prefer larger amounts (more likely to be totals)
            if (value >= 100) {
                candidate.score += 3;
            }
            if (value >= 1000) {
                candidate.score += 2;
            }

            // Skip very small numbers (likely quantities or percentages)
            if (value < 1 && value > 0) {
                candidate.score -= 10;
            }

            debugLog(QString("  Candidate: value=%1, score=%2, line: %3")
                .arg(value).arg(candidate.score).arg(line));

            lineCandidates.append(candidate);
        }
        return lineCandidates;
    };

    for (int lineIdx = 0; lineIdx < lines.size(); ++lineIdx) {
        const QString line = lines[lineIdx].trimmed();
        if (line.isEmpty()) {
            continue;
        }

        // Check for positive labels
        int baseScore = 0;
        QStringList matchedLabels;
        for (const QString &label : positiveLabels) {
            if (line.contains(label, Qt::CaseInsensitive)) {
                // Exact match at line start gets higher score
                if (line.startsWith(label, Qt::CaseInsensitive)) {
                    baseScore += 10;
                    matchedLabels << QString("%1(start)").arg(label);
                } else {
                    baseScore += 5;
                    matchedLabels << label;
                }
            }
        }

        if (baseScore == 0) {
            continue;
        }

        debugLog(QString("  Found label match on line: '%1', score=%2, labels=%3")
            .arg(line).arg(baseScore).arg(matchedLabels.join(",")));

        // Check for negative labels (reduce score)
        QStringList matchedNegLabels;
        for (const QString &negLabel : negLabels) {
            if (line.contains(negLabel, Qt::CaseInsensitive)) {
                baseScore -= 8;
                matchedNegLabels << negLabel;
            }
        }

        // Extract numbers from current line
        QList<AmountCandidate> lineCandidates = extractNumbersFromLine(line, baseScore, line);
        candidates.append(lineCandidates);

        // If no numbers found on current line, check adjacent lines (OCR table structure)
        if (lineCandidates.isEmpty()) {
            debugLog(QString("  No numbers on label line, checking adjacent lines..."));

            // Check previous 2 lines
            for (int offset = 1; offset <= 2; ++offset) {
                if (lineIdx - offset >= 0) {
                    const QString prevLine = lines[lineIdx - offset].trimmed();
                    if (!prevLine.isEmpty()) {
                        // Reduce score for adjacent lines
                        int adjScore = baseScore - offset * 2;
                        debugLog(QString("  Checking previous line %1: '%2'").arg(-offset).arg(prevLine));
                        QList<AmountCandidate> adjCandidates = extractNumbersFromLine(prevLine, adjScore, line);
                        candidates.append(adjCandidates);
                    }
                }
            }

            // Check next 2 lines
            for (int offset = 1; offset <= 2; ++offset) {
                if (lineIdx + offset < lines.size()) {
                    const QString nextLine = lines[lineIdx + offset].trimmed();
                    if (!nextLine.isEmpty()) {
                        int adjScore = baseScore - offset * 2;
                        debugLog(QString("  Checking next line %1: '%2'").arg(offset).arg(nextLine));
                        QList<AmountCandidate> adjCandidates = extractNumbersFromLine(nextLine, adjScore, line);
                        candidates.append(adjCandidates);
                    }
                }
            }
        }
    }

    // Return highest scoring candidate
    if (candidates.isEmpty()) {
        debugLog("  No candidates found!");
        return 0.0;
    }

    std::sort(candidates.begin(), candidates.end(), [](const AmountCandidate &a, const AmountCandidate &b) {
        return a.score > b.score;
    });

    debugLog(QString("  Top 3 candidates:"));
    for (int i = 0; i < qMin(3, candidates.size()); ++i) {
        debugLog(QString("    #%1: value=%2, score=%3, context=%4")
            .arg(i+1).arg(candidates[i].value).arg(candidates[i].score).arg(candidates[i].context));
    }

    debugLog(QString("  SELECTED: %1 (score %2)").arg(candidates.first().value).arg(candidates.first().score));

    return candidates.first().value;
}

QJsonObject choosePrimaryObject(const QJsonObject &json)
{
    if (json.contains(QStringLiteral("data")) && json.value(QStringLiteral("data")).isObject()) {
        return json.value(QStringLiteral("data")).toObject();
    }
    if (json.contains(QStringLiteral("invoice")) && json.value(QStringLiteral("invoice")).isObject()) {
        return json.value(QStringLiteral("invoice")).toObject();
    }
    if (json.contains(QStringLiteral("result")) && json.value(QStringLiteral("result")).isObject()) {
        return json.value(QStringLiteral("result")).toObject();
    }
    return json;
}

double extractTaxRateFromText(const QString &rawText)
{
    if (rawText.isEmpty()) return 0.0;

    debugLog("extractTaxRateFromText called");

    // Common Chinese tax rates (小规模纳税人: 3%, 一般纳税人: 6%, 9%, 13%)
    // Also support half rates for some special cases
    const QList<double> validTaxRates = {3.0, 6.0, 9.0, 13.0, 1.5, 0.5, 0.03, 0.06, 0.09, 0.13};
    auto isValidTaxRate = [&](double rate) -> bool {
        for (double valid : validTaxRates) {
            if (qAbs(rate - valid) < 0.01) return true;
        }
        return false;
    };

    const QStringList lines = rawText.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);
    QRegularExpression pctRe(QStringLiteral(R"((\d{1,2}(?:\.\d+)?)\s*[％%])"));  // 9%, 9％
    QRegularExpression pctReReversed(QStringLiteral(R"([％%]\s*(\d{1,2}(?:\.\d+)?))"));  // %9, ％9 (OCR misread)
    QRegularExpression labelRe(QStringLiteral(R"((?:税率|征收率)[：:\s]*([0-9]{1,2}(?:\.\d+)?))"));

    for (const QString &lineRaw : lines) {
        const QString line = lineRaw.trimmed();
        if (line.isEmpty()) continue;

        // Check for explicit "税率:" pattern
        QRegularExpressionMatch m1 = labelRe.match(line);
        if (m1.hasMatch()) {
            bool ok = false;
            double v = m1.captured(1).toDouble(&ok);
            if (ok && isValidTaxRate(v)) {
                debugLog(QString("  Found tax rate via label: %1% from line: %2").arg(v).arg(line));
                return v;
            }
        }

        // Check for lines containing "税率" with % sign
        if (line.contains(QStringLiteral("税率")) || line.contains(QStringLiteral("征收率"))) {
            QRegularExpressionMatch m2 = pctRe.match(line);
            if (m2.hasMatch()) {
                bool ok = false;
                double v = m2.captured(1).toDouble(&ok);
                if (ok && isValidTaxRate(v)) {
                    debugLog(QString("  Found tax rate in tax line: %1% from line: %2").arg(v).arg(line));
                    return v;
                }
            }
            // Also try reversed format (%9 instead of 9%) - but validate
            QRegularExpressionMatch m2r = pctReReversed.match(line);
            if (m2r.hasMatch()) {
                bool ok = false;
                double v = m2r.captured(1).toDouble(&ok);
                if (ok && isValidTaxRate(v)) {
                    debugLog(QString("  Found tax rate (reversed) in tax line: %1% from line: %2").arg(v).arg(line));
                    return v;
                }
            }
        }
    }

    // Fallback: find any percentage in text (must be valid tax rate)
    QRegularExpressionMatch m = pctRe.match(rawText);
    if (m.hasMatch()) {
        bool ok = false;
        double v = m.captured(1).toDouble(&ok);
        if (ok && isValidTaxRate(v)) {
            debugLog(QString("  Found tax rate fallback: %1%").arg(v));
            return v;
        }
    }

    // Fallback: try reversed percentage format (%9 instead of 9%)
    // IMPORTANT: OCR often misreads %6 as %9, so %9 should be treated as 6%
    QRegularExpressionMatch mr = pctReReversed.match(rawText);
    if (mr.hasMatch()) {
        bool ok = false;
        double v = mr.captured(1).toDouble(&ok);
        if (ok) {
            // %9 is almost always a misread %6 (OCR confuses 6 and 9)
            if (qAbs(v - 9.0) < 0.01) {
                debugLog(QString("  Found %9, treating as 6%% due to OCR confusion (6 vs 9)"));
                return 6.0;
            }
            // Other reversed formats like %6, %3, %13 are valid
            if (isValidTaxRate(v)) {
                debugLog(QString("  Found tax rate fallback (reversed): %1%").arg(v));
                return v;
            }
        }
    }

    debugLog("  No tax rate found");
    return 0.0;
}

// Extract tax amount from OCR table structure where "税额" header and values are on separate lines
// OCR output often looks like:
//   合
//   计
//   ￥87.09    <- amount without tax (first currency value after 合计)
//   ￥2.61     <- tax amount (second currency value after 合计)
double extractTaxAmountFromTable(const QString &rawText)
{
    if (rawText.isEmpty()) return 0.0;

    debugLog("extractTaxAmountFromTable called");

    const QStringList lines = rawText.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);

    // Find the "合计" section - tax values usually follow
    int hejiIndex = -1;
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();
        if (line == QStringLiteral("合") || line == QStringLiteral("计") ||
            line == QStringLiteral("合计")) {
            hejiIndex = i;
            break;
        }
    }

    if (hejiIndex < 0) {
        debugLog("  No '合计' found in table");
        return 0.0;
    }

    debugLog(QString("  Found '合计' at line index %1").arg(hejiIndex));

    // Collect currency values after 合计
    QList<double> currencyValues;
    QRegularExpression currencyRe(QStringLiteral(R"([￥¥]\s*([\d,]+\.?\d*))"));

    for (int i = hejiIndex; i < lines.size() && currencyValues.size() < 4; ++i) {
        QString line = lines[i].trimmed();
        QRegularExpressionMatch match = currencyRe.match(line);
        if (match.hasMatch()) {
            bool ok = false;
            double value = match.captured(1).remove(',').toDouble(&ok);
            if (ok && value > 0) {
                currencyValues.append(value);
                debugLog(QString("  Found currency value: %1 on line: %2").arg(value).arg(line));
            }
        }
    }

    if (currencyValues.size() >= 2) {
        // First value is usually amount without tax, second is tax amount
        // Tax amount is typically smaller
        double taxAmount = currencyValues[1];
        debugLog(QString("  Tax amount from table structure: %1").arg(taxAmount));
        return taxAmount;
    }

    debugLog("  Not enough currency values found after '合计'");
    return 0.0;
}

// Extract amount without tax from table structure (first currency value after 合计)
double extractAmountWithoutTaxFromTable(const QString &rawText)
{
    if (rawText.isEmpty()) return 0.0;

    const QStringList lines = rawText.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);

    int hejiIndex = -1;
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();
        if (line == QStringLiteral("合") || line == QStringLiteral("计") ||
            line == QStringLiteral("合计")) {
            hejiIndex = i;
            break;
        }
    }

    if (hejiIndex < 0) return 0.0;

    QRegularExpression currencyRe(QStringLiteral(R"([￥¥]\s*([\d,]+\.?\d*))"));

    for (int i = hejiIndex; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();
        QRegularExpressionMatch match = currencyRe.match(line);
        if (match.hasMatch()) {
            bool ok = false;
            double value = match.captured(1).remove(',').toDouble(&ok);
            if (ok && value > 0) {
                return value;
            }
        }
    }

    return 0.0;
}

void reconcileAmounts(InvoiceData &invoice)
{
    // Derive totalAmount from other amounts
    if (invoice.totalAmount <= 0.0 && invoice.amountWithoutTax > 0.0 && invoice.taxAmount > 0.0) {
        invoice.totalAmount = invoice.amountWithoutTax + invoice.taxAmount;
    }

    // Derive amountWithoutTax from total and tax
    if (invoice.amountWithoutTax <= 0.0 && invoice.totalAmount > 0.0 && invoice.taxAmount > 0.0
        && invoice.totalAmount >= invoice.taxAmount) {
        invoice.amountWithoutTax = invoice.totalAmount - invoice.taxAmount;
    }

    // Derive taxAmount from total and amount without tax
    if (invoice.taxAmount <= 0.0 && invoice.totalAmount > 0.0 && invoice.amountWithoutTax > 0.0
        && invoice.totalAmount >= invoice.amountWithoutTax) {
        invoice.taxAmount = invoice.totalAmount - invoice.amountWithoutTax;
    }

    // Derive taxRate from tax amount and base amount
    if (invoice.taxRate <= 0.0 && invoice.taxAmount > 0.0 && invoice.amountWithoutTax > 0.0) {
        invoice.taxRate = (invoice.taxAmount / invoice.amountWithoutTax) * 100.0;
    }

    // Normalize tax rate (if < 1, assume it's decimal form)
    if (invoice.taxRate > 0.0 && invoice.taxRate <= 1.0) {
        invoice.taxRate *= 100.0;
    }

    // Validate tax rate range
    if (invoice.taxRate > 100.0 || invoice.taxRate < 0.0) {
        invoice.taxRate = 0.0;
    }
}
}

const QString InvoiceRecognizer::INVOICE_PROMPT = R"(
请仔细分析这张发票图片，提取以下信息并以JSON格式返回。

重要提示：
1. 金额必须是纯数字，不要包含货币符号、逗号或空格
2. 日期必须是YYYY-MM-DD格式
3. 如果无法确定某字段，请填null
4. invoiceCategory根据发票内容判断：交通(机票/火车票/出租车/加油/停车等)、住宿(酒店/宾馆等)、餐饮(餐厅/外卖等)、其他

返回格式：
{
    "invoiceType": "发票类型",
    "invoiceNumber": "发票号码",
    "date": "开票日期（YYYY-MM-DD）",
    "invoiceCategory": "分类（交通/住宿/餐饮/其他）",
    "buyerName": "购买方名称",
    "buyerTaxId": "购买方税号",
    "sellerName": "销售方名称",
    "sellerTaxId": "销售方税号",
    "totalAmount": 价税合计金额（纯数字）, 
    "amountWithoutTax": 不含税金额（纯数字）, 
    "taxAmount": 税额（纯数字）, 
    "taxRate": 税率（纯数字，如13表示13%）, 
    "departure": "出发地（交通类发票）",
    "destination": "目的地（交通类发票）",
    "passengerName": "乘客姓名",
    "stayDays": 入住天数,
    "items": [
        {
            "description": "货物/服务名称",
            "quantity": 数量,
            "unitPrice": 单价,
            "amount": 金额
        }
    ]
}

注意：
1. totalAmount是最重要的字段，请务必准确提取价税合计金额
2. invoiceCategory需要根据发票的实际内容判断，例如：
   - 机票、火车票、出租车发票、加油发票、停车费发票 -> 交通
   - 酒店、宾馆、民宿发票 -> 住宿
   - 餐厅、外卖、餐饮发票 -> 餐饮
   - 无法归类的 -> 其他
)";

InvoiceRecognizer::InvoiceRecognizer(QObject *parent)
    : QObject(parent)
    , m_ocrManager(nullptr)
{
}

void InvoiceRecognizer::setOcrManager(OcrManager *manager)
{
    if (m_ocrManager) {
        disconnect(m_ocrManager, nullptr, this, nullptr);
    }
    m_ocrManager = manager;
    if (m_ocrManager) {
        connect(m_ocrManager, &OcrManager::recognitionFinished,
                this, &InvoiceRecognizer::onOcrFinished);
        connect(m_ocrManager, &OcrManager::recognitionError,
                this, &InvoiceRecognizer::onOcrError);
    }
}

void InvoiceRecognizer::recognize(const QImage &image)
{
    if (!m_ocrManager) {
        emit recognitionError(tr("OCR管理器未设置"));
        return;
    }
    m_isRecognizing = true;
    m_ocrManager->recognizeImage(image, INVOICE_PROMPT);
}

void InvoiceRecognizer::recognizeAsync(const QImage &image)
{
    if (!m_ocrManager) {
        emit recognitionError(tr("OCR管理器未设置"));
        return;
    }

    QtConcurrent::run([this, image]() {
        recognize(image);
    });
}

void InvoiceRecognizer::onOcrFinished(const QJsonObject &result)
{
    if (!m_isRecognizing) {
        return;
    }
    m_isRecognizing = false;
    emit rawOcrReceived(result);

    InvoiceData invoice = parseInvoiceData(result);
    emit recognitionFinished(invoice);
}

void InvoiceRecognizer::onOcrError(const QString &error)
{
    if (!m_isRecognizing) {
        return;
    }
    m_isRecognizing = false;
    emit recognitionError(error);
}

InvoiceData InvoiceRecognizer::parseInvoiceData(const QJsonObject &json)
{
    InvoiceData invoice;
    const QString rawText = extractRawText(json);

    qDebug() << "InvoiceRecognizer: ===== PARSING START =====";
    qDebug() << "InvoiceRecognizer: Input JSON keys:" << json.keys();
    qDebug() << "InvoiceRecognizer: Raw OCR text length:" << rawText.length();
    qDebug() << "InvoiceRecognizer: Raw OCR text preview:" << rawText.left(500);

    QJsonObject normalized = choosePrimaryObject(json);
    qDebug() << "InvoiceRecognizer: Normalized JSON keys:" << normalized.keys();

    if (normalized.contains("text") && normalized["text"].isString()) {
        QJsonObject parsed = parseJsonObjectFromText(normalized["text"].toString());
        if (!parsed.isEmpty()) {
            normalized = parsed;
            qDebug() << "InvoiceRecognizer: Parsed JSON from text field, keys:" << normalized.keys();
        }
    }

    const QJsonValue normalizedValue(normalized);

    // Check if we have structured JSON data or just raw OCR text
    // Local PaddleOCR returns {success, text, boxes} - this is NOT structured invoice data
    // Structured data should contain invoice-specific fields
    bool hasStructuredData = false;
    if (!normalized.isEmpty()) {
        QStringList invoiceFields = {
            QStringLiteral("invoiceType"), QStringLiteral("invoiceNumber"), QStringLiteral("totalAmount"),
            QStringLiteral("buyerName"), QStringLiteral("sellerName"), QStringLiteral("invoiceCategory"),
            QStringLiteral("date"), QStringLiteral("taxAmount"), QStringLiteral("items")
        };
        int matchCount = 0;
        for (const QString &field : invoiceFields) {
            if (normalized.contains(field)) {
                matchCount++;
            }
        }
        // Only treat as structured if we have at least 2 invoice-specific fields
        hasStructuredData = matchCount >= 2;
    }

    auto findText = [&](const QStringList &keys) -> QString {
        QString value = firstNonEmpty(normalized, keys);
        if (!value.isEmpty()) {
            return value;
        }
        QJsonValue deepValue = findValueByKeysDeep(normalizedValue, keys);
        if (deepValue.isString()) {
            const QString text = deepValue.toString().trimmed();
            if (isMeaningfulString(text)) {
                return text;
            }
        }
        return QString();
    };

    auto findNumber = [&](const QStringList &keys) -> double {
        double value = parseAmount(findValueByKeysDeep(normalizedValue, keys));
        if (value > 0.0) {
            return value;
        }
        for (const QString &key : keys) {
            if (normalized.contains(key)) {
                value = parseAmount(normalized.value(key));
                if (value > 0.0) {
                    return value;
                }
            }
        }
        return 0.0;
    };

    // For local OCR (no structured JSON), parse from raw text
    if (!hasStructuredData && !rawText.isEmpty()) {
        debugLog("=== LOCAL OCR PARSING ===");
        debugLog(QString("Raw text (%1 chars):").arg(rawText.length()));
        debugLog(rawText);

        // Extract invoice number
        QRegularExpression invoiceNumRe(QStringLiteral(R"((?:发票号码|发票号|票据号码|No\.?|号码)[：:\s]*([A-Z0-9]+))"));
        QRegularExpressionMatch match = invoiceNumRe.match(rawText);
        if (match.hasMatch()) {
            invoice.invoiceNumber = match.captured(1);
            debugLog(QString("Invoice number: %1").arg(invoice.invoiceNumber));
        }

        // Extract date
        QRegularExpression dateRe(QStringLiteral(R"((\d{4}[-/年]\d{1,2}[-/月]\d{1,2}日?))"));
        match = dateRe.match(rawText);
        if (match.hasMatch()) {
            QString dateStr = match.captured(1);
            dateStr.replace(QStringLiteral("年"), "-").replace(QStringLiteral("月"), "-").replace(QStringLiteral("日"), "");
            dateStr.replace("/", "-");
            invoice.invoiceDate = QDate::fromString(dateStr, "yyyy-M-d");
            debugLog(QString("Date: %1").arg(invoice.invoiceDate.toString()));
        }

        // Extract total amount with candidate scoring
        debugLog("--- Extracting TOTAL AMOUNT ---");
        invoice.totalAmount = extractLabeledAmount(rawText, {
            QStringLiteral("价税合计"), QStringLiteral("合计"), QStringLiteral("总计"),
            QStringLiteral("应付"), QStringLiteral("实付"), QStringLiteral("小写"),
            QStringLiteral("金额"), QStringLiteral("人民币")
        });
        debugLog(QString("Total amount result: %1").arg(invoice.totalAmount));

        // Extract tax amount - try table structure first, then label-based
        debugLog("--- Extracting TAX AMOUNT ---");
        invoice.taxAmount = extractTaxAmountFromTable(rawText);
        if (invoice.taxAmount <= 0) {
            invoice.taxAmount = extractLabeledAmount(rawText, {
                QStringLiteral("税额"), QStringLiteral("税金")
            });
        }
        debugLog(QString("Tax amount result: %1").arg(invoice.taxAmount));

        // Extract amount without tax - try table structure first, then label-based
        debugLog("--- Extracting AMOUNT WITHOUT TAX ---");
        invoice.amountWithoutTax = extractAmountWithoutTaxFromTable(rawText);
        if (invoice.amountWithoutTax <= 0) {
            invoice.amountWithoutTax = extractLabeledAmount(rawText, {
                QStringLiteral("不含税"), QStringLiteral("金额合计"), QStringLiteral("净额")
            });
        }
        debugLog(QString("Amount without tax result: %1").arg(invoice.amountWithoutTax));

        // Extract tax rate
        debugLog("--- Extracting TAX RATE ---");
        invoice.taxRate = extractTaxRateFromText(rawText);
        debugLog(QString("Tax rate result: %1").arg(invoice.taxRate));

        // Reconcile amounts (derive missing values)
        reconcileAmounts(invoice);

        // Validate amounts: tax ≈ amountWithoutTax × taxRate%
        if (invoice.taxRate > 0 && invoice.amountWithoutTax > 0 && invoice.taxAmount > 0) {
            double expectedTax = invoice.amountWithoutTax * invoice.taxRate / 100.0;
            double tolerance = expectedTax * 0.1; // 10% tolerance
            if (qAbs(invoice.taxAmount - expectedTax) > tolerance) {
                debugLog(QString("  WARNING: Tax amount validation failed. Expected ~%1, got %2")
                    .arg(expectedTax).arg(invoice.taxAmount));
            }
        }

        // Check if this is a valid invoice (must have invoice-specific markers)
        // IMPORTANT: Must have invoice number to be a valid invoice
        // 行程单、登机牌、火车票等不是发票，不应混入

        // First check: exclude non-invoice document types
        QStringList nonInvoiceKeywords = {
            QStringLiteral("登机牌"), QStringLiteral("BOARDING PASS"),
            QStringLiteral("行程单"), QStringLiteral("电子客票"),
            QStringLiteral("火车票"), QStringLiteral("车票"),
            QStringLiteral("报销凭证"), QStringLiteral("保险单"),
            QStringLiteral("审批"), QStringLiteral("出差审批"),
            QStringLiteral("航班"), QStringLiteral("FLIGHT"),
            QStringLiteral("车次"), QStringLiteral("座位"),
            QStringLiteral("登机口"), QStringLiteral("GATE"),
            QStringLiteral("起飞时间"), QStringLiteral("DEPTIME")
        };

        for (const QString &keyword : nonInvoiceKeywords) {
            if (rawText.contains(keyword, Qt::CaseInsensitive)) {
                debugLog(QString("  Non-invoice document detected (keyword: %1), marking as invalid").arg(keyword));
                invoice.isValidInvoice = false;
                invoice.invalidReason = QStringLiteral("非发票文档（检测到：%1）").arg(keyword);
                return invoice;
            }
        }

        // Second check: Must have valid invoice number (at least 8 digits/letters)
        // 没有发票号码不可能是发票
        QString invoiceNumPattern = QStringLiteral(R"((\d{8,}|\d{2}[A-Z0-9]{10,}))");
        QRegularExpression numRe(invoiceNumPattern);
        bool hasValidInvoiceNumber = numRe.match(invoice.invoiceNumber).hasMatch();

        if (!hasValidInvoiceNumber) {
            debugLog("  No valid invoice number found, marking as invalid");
            invoice.isValidInvoice = false;
            invoice.invalidReason = QStringLiteral("未检测到有效发票号码");
            return invoice;
        }

        // Third check: Must have invoice-specific keywords
        bool hasInvoiceKeywords = rawText.contains(QStringLiteral("发票")) ||
                                  rawText.contains(QStringLiteral("税务局")) ||
                                  rawText.contains(QStringLiteral("价税合计")) ||
                                  rawText.contains(QStringLiteral("税率/征收率")) ||
                                  rawText.contains(QStringLiteral("开票人"));

        if (!hasInvoiceKeywords) {
            debugLog("  Missing invoice-specific keywords, marking as invalid");
            invoice.isValidInvoice = false;
            invoice.invalidReason = QStringLiteral("缺少发票特征关键词");
            return invoice;
        }

        // Passed all checks - this is a valid invoice
        invoice.isValidInvoice = true;
        debugLog("  Valid invoice detected, proceeding with extraction");

        // Extract seller and buyer names using improved patterns
        // The OCR often splits "销售方信息" and "名称：" across lines
        QStringList lines = rawText.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);
        for (int i = 0; i < lines.size(); ++i) {
            QString line = lines[i].trimmed();

            // Seller name: look for "销售方信息" followed by "名称："
            if (line.contains(QStringLiteral("销售方")) && i + 1 < lines.size()) {
                QString nextLine = lines[i + 1].trimmed();
                if (nextLine.startsWith(QStringLiteral("名称："))) {
                    invoice.sellerName = nextLine.mid(3).trimmed();
                } else if (nextLine.contains(QStringLiteral("名称："))) {
                    int pos = nextLine.indexOf(QStringLiteral("名称："));
                    invoice.sellerName = nextLine.mid(pos + 3).trimmed();
                }
            }
            // Alternative: line contains "名称：XXX公司" directly
            if (line.contains(QStringLiteral("销售方")) || line == QStringLiteral("名称：")) {
                if (i + 1 < lines.size()) {
                    QString nextLine = lines[i + 1].trimmed();
                    // Check if it looks like a company name
                    if (nextLine.contains(QStringLiteral("公司")) ||
                        nextLine.contains(QStringLiteral("有限")) ||
                        nextLine.contains(QStringLiteral("科技")) ||
                        nextLine.contains(QStringLiteral("酒店"))) {
                        // Avoid picking buyer name again
                        if (invoice.sellerName.isEmpty() && nextLine != invoice.buyerName) {
                            invoice.sellerName = nextLine;
                        }
                    }
                }
            }

            // Buyer name: similar logic
            if (line.contains(QStringLiteral("购买方")) && i + 1 < lines.size()) {
                QString nextLine = lines[i + 1].trimmed();
                if (nextLine.startsWith(QStringLiteral("名称："))) {
                    invoice.buyerName = nextLine.mid(3).trimmed();
                } else if (nextLine.contains(QStringLiteral("名称："))) {
                    int pos = nextLine.indexOf(QStringLiteral("名称："));
                    invoice.buyerName = nextLine.mid(pos + 3).trimmed();
                }
            }
        }

        // Fallback: use regex patterns
        if (invoice.sellerName.isEmpty()) {
            QRegularExpression sellerRe(QStringLiteral(R"(名称[：:]\s*([^\n\r]+(?:公司|酒店|餐厅)[^\n\r]*))"));
            match = sellerRe.match(rawText);
            if (match.hasMatch()) {
                invoice.sellerName = match.captured(1).trimmed();
            }
        }

        if (invoice.buyerName.isEmpty()) {
            QRegularExpression buyerRe(QStringLiteral(R"((?:购买方|付款方)[名称：:\s]*([^\n\r]+))"));
            match = buyerRe.match(rawText);
            if (match.hasMatch()) {
                invoice.buyerName = match.captured(1).trimmed();
            }
        }

        // Detect invoice type from keywords
        if (rawText.contains(QStringLiteral("增值税")) || rawText.contains(QStringLiteral("发票"))) {
            invoice.invoiceType = QStringLiteral("增值税发票");
        }

        // Classify category from content
        InvoiceClassifier classifier;
        invoice.category = classifier.classify(invoice);
        if (invoice.category == InvoiceData::Other) {
            // Try to classify from raw text keywords
            if (rawText.contains(QStringLiteral("酒店")) || rawText.contains(QStringLiteral("住宿"))) {
                invoice.category = InvoiceData::Accommodation;
            } else if (rawText.contains(QStringLiteral("餐饮")) || rawText.contains(QStringLiteral("饭店"))) {
                invoice.category = InvoiceData::Dining;
            } else if (rawText.contains(QStringLiteral("机票")) || rawText.contains(QStringLiteral("火车"))
                       || rawText.contains(QStringLiteral("出租车")) || rawText.contains(QStringLiteral("加油"))) {
                invoice.category = InvoiceData::Transportation;
            }
        }

        qDebug() << "InvoiceRecognizer: Parsed from raw text - Number:" << invoice.invoiceNumber
                 << "Amount:" << invoice.totalAmount << "Category:" << invoice.categoryString();

        return invoice;
    }

    // Structured JSON parsing (for API-based OCR)
    invoice.invoiceType = findText({"invoiceType", "type", "发票类型"});
    invoice.invoiceNumber = findText({"invoiceNumber", "number", "invoiceNo", "发票号码", "发票号", "票据号码"});

    QString dateStr = findText({"date", "invoiceDate", "开票日期", "日期"});
    if (!dateStr.isEmpty()) {
        invoice.invoiceDate = QDate::fromString(dateStr, "yyyy-MM-dd");
        if (!invoice.invoiceDate.isValid()) {
            invoice.invoiceDate = QDate::fromString(dateStr, "yyyy年MM月dd日");
        }
        if (!invoice.invoiceDate.isValid()) {
            invoice.invoiceDate = QDate::fromString(dateStr, "yyyy/MM/dd");
        }
    }

    invoice.buyerName = findText({"buyerName", "buyer", "购买方名称", "购买方"});
    invoice.buyerTaxId = findText({"buyerTaxId", "buyerTaxNo", "购买方税号"});

    invoice.sellerName = findText({"sellerName", "seller", "销售方名称", "销售方"});
    invoice.sellerTaxId = findText({"sellerTaxId", "sellerTaxNo", "销售方税号"});

    invoice.totalAmount = findNumber({
        "totalAmount", "amount", "invoiceAmount", "grandTotal", "payableAmount",
        "价税合计", "合计", "总金额", "金额", "小写"
    });
    if (invoice.totalAmount == 0.0) {
        invoice.totalAmount = extractLabeledAmount(rawText, {
            QStringLiteral("价税合计"), QStringLiteral("合计"), QStringLiteral("总计"),
            QStringLiteral("应付"), QStringLiteral("实付"), QStringLiteral("金额")
        });
    }

    invoice.amountWithoutTax = findNumber({
        "amountWithoutTax", "netAmount", "subtotal", "不含税金额", "金额合计"
    });
    if (invoice.amountWithoutTax == 0.0) {
        invoice.amountWithoutTax = extractLabeledAmount(rawText, {
            QStringLiteral("不含税"), QStringLiteral("金额合计"), QStringLiteral("净额")
        });
    }

    invoice.taxAmount = findNumber({"taxAmount", "tax", "taxTotal", "税额"});
    if (invoice.taxAmount == 0.0) {
        invoice.taxAmount = extractLabeledAmount(rawText, {
            QStringLiteral("税额"), QStringLiteral("税金")
        });
    }

    invoice.taxRate = findNumber({"taxRate", "rate", "税率"});
    if (invoice.taxRate == 0.0) {
        invoice.taxRate = extractTaxRateFromText(rawText);
    }

    // Reconcile amounts
    reconcileAmounts(invoice);

    invoice.departure = findText({"departure", "from", "出发地", "始发地"});
    invoice.destination = findText({"destination", "to", "目的地", "到达地"});
    invoice.passengerName = findText({"passengerName", "passenger", "乘客姓名"});
    invoice.stayDays = static_cast<int>(findNumber({"stayDays", "入住天数", "days"}));

    QJsonValue itemsValue = findValueByKeysDeep(normalizedValue, {"items", "details", "明细"});
    if (itemsValue.isArray()) {
        QJsonArray items = itemsValue.toArray();
        for (const auto &item : items) {
            if (!item.isObject()) {
                continue;
            }
            QJsonObject itemObj = item.toObject();
            InvoiceItem invItem;
            invItem.description = firstNonEmpty(itemObj, {"description", "name", "名称", "项目"});
            invItem.quantity = parseAmount(itemObj.value("quantity"));
            if (invItem.quantity == 0.0) {
                invItem.quantity = parseAmount(itemObj.value("qty"));
            }
            invItem.unitPrice = parseAmount(itemObj.value("unitPrice"));
            if (invItem.unitPrice == 0.0) {
                invItem.unitPrice = parseAmount(itemObj.value("price"));
            }
            invItem.amount = parseAmount(itemObj.value("amount"));
            if (invItem.amount == 0.0) {
                invItem.amount = parseAmount(itemObj.value("total"));
            }
            invoice.items.append(invItem);
        }
    }

    if (invoice.totalAmount == 0.0 && !invoice.items.isEmpty()) {
        double sum = 0.0;
        for (const InvoiceItem &item : invoice.items) {
            sum += item.amount;
        }
        if (sum > 0.0) {
            invoice.totalAmount = sum;
        }
    }

    QString categoryStr = findText({"invoiceCategory", "category", "分类"});
    if (!categoryStr.isEmpty()) {
        InvoiceClassifier classifier;
        invoice.category = classifier.stringToCategory(categoryStr);
        qDebug() << "InvoiceRecognizer: Category from OCR:" << categoryStr << "->" << invoice.categoryString();
    }

    if (invoice.category == InvoiceData::Other) {
        InvoiceClassifier classifier;
        InvoiceData::Category keywordCategory = classifier.classify(invoice);
        if (keywordCategory != InvoiceData::Other) {
            invoice.category = keywordCategory;
            qDebug() << "InvoiceRecognizer: Category from keywords:" << invoice.categoryString();
        }
    }

    qDebug() << "InvoiceRecognizer: Parsed invoice - Number:" << invoice.invoiceNumber
             << "Amount:" << invoice.totalAmount
             << "Seller:" << invoice.sellerName
             << "Category:" << invoice.categoryString();

    return invoice;
}

double InvoiceRecognizer::parseAmount(const QJsonValue &value)
{
    if (value.isDouble()) {
        return value.toDouble();
    }

    QString str = value.toString().trimmed();
    if (str.isEmpty()) {
        return 0.0;
    }

    // Remove currency symbols and common prefixes
    str.remove(QChar(0xFFE5));  // Fullwidth yen
    str.remove(QChar(0x00A5));  // Yen sign
    str.remove('$');
    str.remove(' ');
    str.remove(',');
    str.remove(QStringLiteral("元")).remove(QStringLiteral("人民币"));
    str.remove(QStringLiteral("￥")).remove(QStringLiteral("¥"));

    // Try to find a number - prefer numbers with decimal part for amounts
    QRegularExpression re(QStringLiteral(R"((\d+\.\d{1,2})|(\d+))"));
    QRegularExpressionMatch match = re.match(str);
    if (match.hasMatch()) {
        bool ok = false;
        double result = match.captured(0).toDouble(&ok);
        if (ok && result >= 0) {
            return result;
        }
    }

    return 0.0;
}
