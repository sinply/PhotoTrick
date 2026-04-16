#include "OcrParser.h"
#include <QRegularExpression>
#include <QJsonArray>
#include <algorithm>
#include <functional>

namespace OcrParser {

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

QJsonDocument tryParseJson(QString text)
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
    if (!doc.isNull()) {
        return doc;
    }

    int firstBrace = text.indexOf('{');
    int lastBrace = text.lastIndexOf('}');
    if (firstBrace >= 0 && lastBrace > firstBrace) {
        doc = QJsonDocument::fromJson(text.mid(firstBrace, lastBrace - firstBrace + 1).toUtf8(), &err);
        if (!doc.isNull()) {
            return doc;
        }
    }

    int firstBracket = text.indexOf('[');
    int lastBracket = text.lastIndexOf(']');
    if (firstBracket >= 0 && lastBracket > firstBracket) {
        doc = QJsonDocument::fromJson(text.mid(firstBracket, lastBracket - firstBracket + 1).toUtf8(), &err);
        if (!doc.isNull()) {
            return doc;
        }
    }

    return QJsonDocument();
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

    // Two-pass: exact match first, then substring match
    std::function<QJsonValue(const QJsonValue &, bool)> walk = [&](const QJsonValue &node, bool exactOnly) -> QJsonValue {
        if (node.isObject()) {
            const QJsonObject obj = node.toObject();

            // First: exact matches
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

            // Second: substring match (only if not exact-only)
            if (!exactOnly) {
                for (auto it = obj.begin(); it != obj.end(); ++it) {
                    const QString keyNorm = normalizeKey(it.key());
                    bool match = false;
                    for (const QString &target : normalizedTargets) {
                        // Only allow longer key to contain target
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

double parseNumber(const QJsonValue &value)
{
    if (value.isDouble()) {
        return value.toDouble();
    }
    QString str = value.toString().trimmed();
    if (str.isEmpty()) {
        return 0.0;
    }
    // Remove currency symbols and formatting
    str.remove(QChar(0xFFE5));  // ￥ fullwidth
    str.remove(QChar(0x00A5));  // ¥ halfwidth
    str.remove('$');
    str.remove(' ');
    str.remove(',');
    str.remove(QStringLiteral("元"));
    QRegularExpression re(QStringLiteral(R"([-+]?\d+\.?\d*)"));
    QRegularExpressionMatch match = re.match(str);
    if (match.hasMatch()) {
        return match.captured(0).toDouble();
    }
    return 0.0;
}

double extractLabeledAmount(const QString &rawText,
                            const QStringList &positiveLabels,
                            const QStringList &negativeLabels)
{
    if (rawText.isEmpty() || positiveLabels.isEmpty()) {
        return 0.0;
    }

    struct AmountCandidate {
        double value = 0.0;
        int score = 0;
        QString context;
    };

    const QStringList lines = rawText.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);
    QList<AmountCandidate> candidates;

    // Default negative keywords that indicate non-amount numbers
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

    // Helper to extract numbers from a line
    auto extractNumbersFromLine = [&](const QString &line, int baseScore) -> QList<AmountCandidate> {
        QList<AmountCandidate> lineCandidates;
        QRegularExpressionMatchIterator it = numRe.globalMatch(line);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString numStr = match.captured(2);
            bool ok = false;
            const double value = QString(numStr).remove(',').toDouble(&ok);
            if (!ok || value <= 0) {
                continue;
            }

            AmountCandidate candidate;
            candidate.value = value;
            candidate.context = line;
            candidate.score = baseScore;

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
            if (value >= 100) candidate.score += 3;
            if (value >= 1000) candidate.score += 2;

            // Skip very small numbers (likely quantities or percentages)
            if (value < 1 && value > 0) candidate.score -= 10;

            lineCandidates.append(candidate);
        }
        return lineCandidates;
    };

    for (int lineIdx = 0; lineIdx < lines.size(); ++lineIdx) {
        const QString line = lines[lineIdx].trimmed();
        if (line.isEmpty()) continue;

        // Check for positive labels
        int baseScore = 0;
        for (const QString &label : positiveLabels) {
            if (line.contains(label, Qt::CaseInsensitive)) {
                baseScore += line.startsWith(label, Qt::CaseInsensitive) ? 10 : 5;
                // "小写" and "价税合计" are strong indicators of total amount
                if (label == QStringLiteral("小写") || label == QStringLiteral("价税合计")) {
                    baseScore += 5;
                }
            }
        }

        if (baseScore == 0) continue;

        // Check for negative labels (reduce score)
        for (const QString &negLabel : negLabels) {
            if (line.contains(negLabel, Qt::CaseInsensitive)) {
                baseScore -= 8;
            }
        }

        // Extract numbers from current line
        QList<AmountCandidate> lineCandidates = extractNumbersFromLine(line, baseScore);
        candidates.append(lineCandidates);

        // If no numbers found on current line, check adjacent lines (OCR table structure)
        if (lineCandidates.isEmpty()) {
            // Check previous 2 lines
            for (int offset = 1; offset <= 2; ++offset) {
                if (lineIdx - offset >= 0) {
                    const QString prevLine = lines[lineIdx - offset].trimmed();
                    if (!prevLine.isEmpty()) {
                        int adjScore = baseScore - offset * 2;
                        candidates.append(extractNumbersFromLine(prevLine, adjScore));
                    }
                }
            }

            // Check next 2 lines
            for (int offset = 1; offset <= 2; ++offset) {
                if (lineIdx + offset < lines.size()) {
                    const QString nextLine = lines[lineIdx + offset].trimmed();
                    if (!nextLine.isEmpty()) {
                        int adjScore = baseScore - offset * 2;
                        candidates.append(extractNumbersFromLine(nextLine, adjScore));
                    }
                }
            }
        }
    }

    if (candidates.isEmpty()) return 0.0;

    std::sort(candidates.begin(), candidates.end(), [](const AmountCandidate &a, const AmountCandidate &b) {
        if (a.score != b.score) {
            return a.score > b.score;
        }
        // When scores are equal, prefer larger values for total amounts
        return a.value > b.value;
    });

    return candidates.first().value;
}

} // namespace OcrParser
