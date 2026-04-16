#include "InvoiceRecognizer.h"
#include "../core/OcrManager.h"
#include "../classifiers/InvoiceClassifier.h"
#include "../utils/OcrParser.h"
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

// Shared non-invoice document keywords
// Used to detect documents that are NOT invoices (itineraries, tickets, etc.)
const QStringList NON_INVOICE_KEYWORDS = {
    // 登机牌/机票
    QStringLiteral("登机牌"), QStringLiteral("BOARDING PASS"),
    QStringLiteral("航班号"), QStringLiteral("FLIGHT NO"),
    QStringLiteral("承运人"), QStringLiteral("CARRIER"),
    QStringLiteral("起飞时间"), QStringLiteral("DEPTIME"),
    QStringLiteral("到达时间"), QStringLiteral("ARRTIME"),
    QStringLiteral("登机口"),  // GATE too generic
    QStringLiteral("座位号"),  // SEAT too generic
    QStringLiteral("舱位"),  // CLASS too generic
    QStringLiteral("客票级别"), QStringLiteral("FARE BASIS"),
    // 行程单/电子客票
    QStringLiteral("行程单"), QStringLiteral("ITINERARY"),
    QStringLiteral("电子客票"), QStringLiteral("E-TICKET"),
    QStringLiteral("客票号码"), QStringLiteral("TICKET NO"),
    QStringLiteral("印刷序号"), QStringLiteral("SERIAL NO"),
    QStringLiteral("销售单位代号"), QStringLiteral("AGENT CODE"),
    QStringLiteral("填开单位"), QStringLiteral("ISSUED BY"),
    QStringLiteral("旅客姓名"), QStringLiteral("PASSENGER"),
    QStringLiteral("始发站"),  // FROM too generic
    QStringLiteral("目的站"),  // TO too generic
    QStringLiteral("航线"), QStringLiteral("ROUTE"),
    QStringLiteral("票价"), QStringLiteral("FARE"),
    QStringLiteral("燃油附加费"), QStringLiteral("FUEL SURCHARGE"),
    QStringLiteral("机场建设费"), QStringLiteral("AIRPORT TAX"),
    QStringLiteral("民航发展基金"), QStringLiteral("CIVIL AVIATION"),
    QStringLiteral("保险费"), QStringLiteral("INSURANCE"),
    QStringLiteral("合计金额"), QStringLiteral("TOTAL FARE"),
    QStringLiteral("电子客票号码"),
    // 火车票
    QStringLiteral("火车票"), QStringLiteral("TRAIN TICKET"),
    QStringLiteral("车次"), QStringLiteral("TRAIN NO"),
    QStringLiteral("车厢"), QStringLiteral("CAR NO"),
    QStringLiteral("开往"),  // TO too generic
    QStringLiteral("检票口"),  // GATE too generic
    // 汽车票/出租车
    QStringLiteral("车票"),  // TICKET too generic
    QStringLiteral("出租车"), QStringLiteral("TAXI"),
    QStringLiteral("网约车"), QStringLiteral("RIDE"),
    // 其他
    QStringLiteral("报销凭证"), QStringLiteral("保险单"),
    QStringLiteral("审批"), QStringLiteral("出差审批"),
    QStringLiteral("航班"),  // FLIGHT too generic
    // 费用报告/报销单
    QStringLiteral("费用报告"), QStringLiteral("EXPENSE REPORT"),
    QStringLiteral("报销单"), QStringLiteral("EXPENSE CLAIM"),
    QStringLiteral("费用清单"), QStringLiteral("EXPENSE SHEET"),
    QStringLiteral("费用明细"), QStringLiteral("EXPENSE DETAIL"),
    // 国际航线常见词
    QStringLiteral("PASSENGER NAME"),
    QStringLiteral("FLIGHT DATE"),
    QStringLiteral("ISSUE DATE"),
    QStringLiteral("NOT TRANSFERABLE"),
    QStringLiteral("E-TICKET RECEIPT")
};

// Ride-hailing platform keywords for detecting ride-hailing itineraries
const QStringList RIDE_HAILING_KEYWORDS = {
    QStringLiteral("高德打车"), QStringLiteral("滴滴"), QStringLiteral("嘀嘀"),
    QStringLiteral("网约车"), QStringLiteral("曹操出行"), QStringLiteral("神州专车"),
    QStringLiteral("首汽约车"), QStringLiteral("T3出行"), QStringLiteral("美团打车"),
    QStringLiteral("花小猪"), QStringLiteral("嘀嗒出行"), QStringLiteral("享道出行"),
    QStringLiteral("如祺出行")
};

// Check if sellerName indicates a real airline company
// Must be more specific than just "航空" to avoid false positives like "航空食品有限公司"
bool isAirlineCompany(const QString &sellerName)
{
    // Must have "航空" AND end with airline company suffixes
    if (sellerName.contains(QStringLiteral("航空"))) {
        // Airline companies typically end with these suffixes
        if (sellerName.endsWith(QStringLiteral("航空公司")) ||
            sellerName.endsWith(QStringLiteral("航空股份有限公司")) ||
            sellerName.endsWith(QStringLiteral("航空有限公司"))) {
            return true;
        }
        // Check for specific airline names (without requiring suffix)
        const QStringList knownAirlines = {
            QStringLiteral("南方航空"), QStringLiteral("东方航空"), QStringLiteral("国际航空"),
            QStringLiteral("厦门航空"), QStringLiteral("深圳航空"), QStringLiteral("山东航空"),
            QStringLiteral("四川航空"), QStringLiteral("海南航空"), QStringLiteral("春秋航空"),
            QStringLiteral("吉祥航空"), QStringLiteral("华夏航空"), QStringLiteral("天津航空"),
            QStringLiteral("首都航空"), QStringLiteral("祥鹏航空"), QStringLiteral("西部航空"),
            QStringLiteral("乌鲁木齐航空"), QStringLiteral("北部湾航空"), QStringLiteral("长安航空"),
            QStringLiteral("金鹏航空"), QStringLiteral("福州航空"), QStringLiteral("桂林航空"),
            QStringLiteral("多彩航空"), QStringLiteral("河北航空"), QStringLiteral("江西航空"),
            QStringLiteral("青岛航空"), QStringLiteral("瑞丽航空"), QStringLiteral("东海航空"),
            QStringLiteral("九元航空"), QStringLiteral("长龙航空"), QStringLiteral("昆明航空"),
            QStringLiteral("西藏航空"), QStringLiteral("奥凯航空"), QStringLiteral("红土航空"),
            QStringLiteral("AIR CHINA"), QStringLiteral("CHINA EASTERN"), QStringLiteral("CHINA SOUTHERN")
        };
        for (const QString &airline : knownAirlines) {
            if (sellerName.contains(airline, Qt::CaseInsensitive)) {
                return true;
            }
        }
    }
    return false;
}

// Check if document is a ride-hailing itinerary
bool isRideHailingItinerary(const QString &sellerName, const QString &rawText)
{
    // Check sellerName first
    for (const QString &keyword : RIDE_HAILING_KEYWORDS) {
        if (sellerName.contains(keyword, Qt::CaseInsensitive)) {
            return true;
        }
    }
    // Also check raw text as fallback
    for (const QString &keyword : RIDE_HAILING_KEYWORDS) {
        if (rawText.contains(keyword, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

// Check if document is an airline ticket/itinerary
bool isAirlineTicketDoc(const QString &sellerName, const QString &rawText, const QJsonArray &items = QJsonArray())
{
    // Check sellerName for airline company
    if (isAirlineCompany(sellerName)) {
        return true;
    }

    // Check items for "机票款" description
    for (const QJsonValue &item : items) {
        if (item.isObject()) {
            QString desc = item.toObject().value("description").toString();
            if (desc.contains(QStringLiteral("机票款")) || desc.contains(QStringLiteral("机票"))) {
                return true;
            }
        }
    }

    return false;
}

// Check for non-invoice document keywords in text
bool containsNonInvoiceKeyword(const QString &text)
{
    for (const QString &keyword : NON_INVOICE_KEYWORDS) {
        if (text.contains(keyword, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

}

static double extractTaxRateFromText(const QString &rawText)
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
    // Normal format: "9%" means 9% tax rate
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
    // This means OCR misread the position AND likely misread 6 as 9
    // %9 usually means the original was 6% (OCR confused 6->9 and % position)
    QRegularExpressionMatch mr = pctReReversed.match(rawText);
    if (mr.hasMatch()) {
        bool ok = false;
        double v = mr.captured(1).toDouble(&ok);
        if (ok) {
            // %9 is almost always a misread of 6% (OCR confuses 6 and 9 visually)
            if (qAbs(v - 9.0) < 0.01) {
                debugLog(QString("  Found reversed %9, correcting to 6%% (OCR 6/9 confusion)"));
                return 6.0;
            }
            // Other values like %6, %3, %13 are valid as-is
            if (isValidTaxRate(v)) {
                debugLog(QString("  Found tax rate fallback (reversed %%%1 -> %1%%)").arg(v));
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

const QString InvoiceRecognizer::INVOICE_PROMPT = R"(
请仔细分析这张发票图片，提取以下信息并以JSON格式返回。

重要提示：
1. 金额必须是纯数字，不要包含货币符号、逗号或空格
2. 日期必须是YYYY-MM-DD格式
3. 如果无法确定某字段，请填null
4. invoiceCategory根据发票内容判断：交通(机票/火车票/出租车/加油/停车等)、住宿(酒店/宾馆等)、餐饮(餐厅/外卖等)、其他
5. documentType字段用于判断文档类型：
   - 如果是正规发票（增值税发票、电子发票等），填"invoice"
   - 如果是行程单、登机牌、火车票，填"itinerary"
   - 如果是费用报告、报销单、费用清单等非发票文档，填"expense_report"
   - 如果是其他类型文档，填具体类型名称

返回格式：
{
    "documentType": "文档类型（invoice/itinerary/expense_report/其他）",
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
    const QString rawText = OcrParser::extractRawText(json);

    qDebug() << "InvoiceRecognizer: ===== PARSING START =====";
    qDebug() << "InvoiceRecognizer: Input JSON keys:" << json.keys();
    qDebug() << "InvoiceRecognizer: Raw OCR text length:" << rawText.length();
    qDebug() << "InvoiceRecognizer: Raw OCR text preview:" << rawText.left(500);

    QJsonObject normalized = OcrParser::choosePrimaryObject(json);
    qDebug() << "InvoiceRecognizer: Normalized JSON keys:" << normalized.keys();
    debugLog(QString("Normalized JSON keys: %1").arg(normalized.keys().join(", ")));
    debugLog(QString("Full normalized JSON: %1").arg(QJsonDocument(normalized).toJson(QJsonDocument::Compact).left(2000)));

    // Try to parse JSON from text field if present
    if (normalized.contains("text") && normalized["text"].isString()) {
        QString textContent = normalized["text"].toString();
        debugLog(QString("Found 'text' field, content preview: %1").arg(textContent.left(200)));

        QJsonObject parsed = OcrParser::parseJsonObjectFromText(textContent);
        if (!parsed.isEmpty()) {
            normalized = parsed;
            qDebug() << "InvoiceRecognizer: Parsed JSON from text field, keys:" << normalized.keys();
            debugLog(QString("Parsed JSON from text field, keys: %1").arg(normalized.keys().join(", ")));
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
        QStringList matchedFields;
        for (const QString &field : invoiceFields) {
            if (normalized.contains(field)) {
                matchCount++;
                matchedFields << field;
            }
        }
        // Only treat as structured if we have at least 2 invoice-specific fields
        hasStructuredData = matchCount >= 2;
        debugLog(QString("hasStructuredData check: %1 fields matched (%2), result=%3")
            .arg(matchCount).arg(matchedFields.join(", ")).arg(hasStructuredData));
    }

    auto findText = [&](const QStringList &keys) -> QString {
        QString value = OcrParser::firstNonEmpty(normalized, keys);
        if (!value.isEmpty()) {
            return value;
        }
        QJsonValue deepValue = OcrParser::findValueByKeysDeep(normalizedValue, keys);
        if (deepValue.isString()) {
            const QString text = deepValue.toString().trimmed();
            if (OcrParser::isMeaningfulString(text)) {
                return text;
            }
        }
        return QString();
    };

    auto findNumber = [&](const QStringList &keys) -> double {
        double value = parseAmount(OcrParser::findValueByKeysDeep(normalizedValue, keys));
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
        invoice.totalAmount = OcrParser::extractLabeledAmount(rawText, {
            QStringLiteral("价税合计"), QStringLiteral("合计"), QStringLiteral("总计"),
            QStringLiteral("应付"), QStringLiteral("实付"), QStringLiteral("小写"),
            QStringLiteral("金额"), QStringLiteral("人民币")
        });
        debugLog(QString("Total amount result: %1").arg(invoice.totalAmount));

        // Extract tax amount - try table structure first, then label-based
        debugLog("--- Extracting TAX AMOUNT ---");
        invoice.taxAmount = extractTaxAmountFromTable(rawText);
        if (invoice.taxAmount <= 0) {
            invoice.taxAmount = OcrParser::extractLabeledAmount(rawText, {
                QStringLiteral("税额"), QStringLiteral("税金")
            });
        }
        debugLog(QString("Tax amount result: %1").arg(invoice.taxAmount));

        // Extract amount without tax - try table structure first, then label-based
        debugLog("--- Extracting AMOUNT WITHOUT TAX ---");
        invoice.amountWithoutTax = extractAmountWithoutTaxFromTable(rawText);
        if (invoice.amountWithoutTax <= 0) {
            invoice.amountWithoutTax = OcrParser::extractLabeledAmount(rawText, {
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

        // First check: exclude non-invoice document types using shared keywords
        if (containsNonInvoiceKeyword(rawText)) {
            // Find which keyword matched for reporting
            for (const QString &keyword : NON_INVOICE_KEYWORDS) {
                if (rawText.contains(keyword, Qt::CaseInsensitive)) {
                    debugLog(QString("  Non-invoice document detected (keyword: %1), marking as invalid").arg(keyword));
                    invoice.isValidInvoice = false;
                    invoice.invalidReason = QStringLiteral("非发票文档（检测到：%1）").arg(keyword);
                    return invoice;
                }
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
    debugLog("=== STRUCTURED JSON PARSING (API) ===");
    debugLog(QString("Raw text for keyword check (%1 chars):").arg(rawText.length()));
    debugLog(rawText.left(1000));

    // First check: detect non-invoice documents from structured fields
    // 行程单/机票/火车票特征字段检测
    // 注意：API 可能返回固定 schema，字段值可能为 null 或空，需要检查是否有实际值
    auto hasValidValue = [&normalized, this](const QString &key) -> bool {
        if (!normalized.contains(key)) {
            debugLog(QString("  hasValidValue('%1'): key not found").arg(key));
            return false;
        }
        QJsonValue val = normalized.value(key);
        if (val.isNull()) {
            debugLog(QString("  hasValidValue('%1'): value is null").arg(key));
            return false;
        }
        if (val.isString()) {
            QString s = val.toString().trimmed();
            bool valid = !s.isEmpty() && s != "null";
            debugLog(QString("  hasValidValue('%1'): string '%2' -> %3").arg(key, s, valid ? "true" : "false"));
            return valid;
        }
        if (val.isDouble()) {
            bool valid = val.toDouble() != 0.0;
            debugLog(QString("  hasValidValue('%1'): double %2 -> %3").arg(key).arg(val.toDouble()).arg(valid ? "true" : "false"));
            return valid;
        }
        debugLog(QString("  hasValidValue('%1'): other type -> true").arg(key));
        return !val.isNull();
    };

    // First check: documentType field from API response
    // This is the most reliable way to detect non-invoice documents
    if (hasValidValue("documentType")) {
        QString docType = normalized.value("documentType").toString().trimmed().toLower();
        debugLog(QString("documentType field: '%1'").arg(docType));

        if (docType != "invoice" && docType != QStringLiteral("发票")) {
            invoice.isValidInvoice = false;
            if (docType == "itinerary" || docType == QStringLiteral("行程单")) {
                invoice.invalidReason = QStringLiteral("非发票文档（检测到行程单/交通票据特征字段）");
            } else if (docType == "expense_report" || docType == QStringLiteral("费用报告") || docType == QStringLiteral("报销单")) {
                invoice.invalidReason = QStringLiteral("非发票文档（检测到费用报告/报销单）");
            } else {
                invoice.invalidReason = QStringLiteral("非发票文档（文档类型：%1）").arg(docType);
            }
            debugLog(QString("Non-invoice document detected by documentType field: %1").arg(docType));
            return invoice;
        }
    }

    debugLog("Checking itinerary fields:");
    bool hasItineraryFields = hasValidValue("departure") ||
                              hasValidValue("destination") ||
                              hasValidValue("passengerName") ||
                              hasValidValue("passenger") ||
                              hasValidValue("flightNo") ||
                              hasValidValue("trainNo") ||
                              hasValidValue("seatNo") ||
                              hasValidValue("carriage");

    // 检查 sellerName 是否包含打车平台名称
    QString sellerName = hasValidValue("sellerName") ? normalized.value("sellerName").toString().trimmed() : QString();
    debugLog(QString("sellerName for check: '%1'").arg(sellerName));

    // 使用共享函数检测打车平台行程单
    bool detectedRideHailing = isRideHailingItinerary(sellerName, rawText);
    if (detectedRideHailing) {
        debugLog("Detected ride-hailing itinerary");
    }

    // 获取 items 数组用于机票检测
    QJsonArray itemsArray;
    if (normalized.contains("items") && normalized.value("items").isArray()) {
        itemsArray = normalized.value("items").toArray();
    }

    // 使用共享函数检测机票行程单
    bool detectedAirlineTicket = isAirlineTicketDoc(sellerName, rawText, itemsArray);
    if (detectedAirlineTicket) {
        debugLog("Detected airline ticket/itinerary");
    }

    debugLog("Checking invoice fields:");
    // 同时检查是否有发票特征字段
    bool hasInvoiceFields = hasValidValue("invoiceNumber") ||
                            hasValidValue("invoiceNo") ||
                            hasValidValue("invoiceType");

    debugLog(QString("Result: hasItineraryFields=%1, hasInvoiceFields=%2, isRideHailing=%3, isAirline=%4")
             .arg(hasItineraryFields ? "true" : "false")
             .arg(hasInvoiceFields ? "true" : "false")
             .arg(detectedRideHailing ? "true" : "false")
             .arg(detectedAirlineTicket ? "true" : "false"));

    // 判定为非发票的条件：
    // 1. 存在行程单特征且缺少发票特征
    // 2. 打车平台行程单
    // 3. 航空公司机票行程单
    if ((hasItineraryFields && !hasInvoiceFields) || detectedRideHailing || detectedAirlineTicket) {
        invoice.isValidInvoice = false;
        if (detectedRideHailing) {
            invoice.invalidReason = QStringLiteral("非发票文档（检测到打车平台行程单）");
        } else if (detectedAirlineTicket) {
            invoice.invalidReason = QStringLiteral("非发票文档（检测到机票行程单）");
        } else {
            invoice.invalidReason = QStringLiteral("非发票文档（检测到行程单/交通票据特征字段）");
        }
        debugLog(QString("Non-invoice document detected (itinerary=%1, rideHailing=%2, airline=%3), marking as invalid")
                 .arg(hasItineraryFields ? "true" : "false")
                 .arg(detectedRideHailing ? "true" : "false")
                 .arg(detectedAirlineTicket ? "true" : "false"));
        return invoice;
    }

    // Second check: exclude non-invoice document types by keyword in raw text using shared keywords
    if (containsNonInvoiceKeyword(rawText)) {
        // Find which keyword matched for reporting
        for (const QString &keyword : NON_INVOICE_KEYWORDS) {
            if (rawText.contains(keyword, Qt::CaseInsensitive)) {
                invoice.isValidInvoice = false;
                invoice.invalidReason = QStringLiteral("非发票文档（检测到关键词：%1）").arg(keyword);
                debugLog(QString("Non-invoice document detected (keyword: %1), marking as invalid").arg(keyword));
                return invoice;
            }
        }
    }

    // Third check: For API OCR with structured data but no raw text,
    // check for non-invoice keywords in structured field values
    // This catches cases like expense reports where API "hallucinates" invoice structure
    if (rawText.isEmpty() && hasStructuredData) {
        // Collect all string values from the structured data to check
        QStringList structuredTexts;
        for (auto it = normalized.begin(); it != normalized.end(); ++it) {
            if (it.value().isString()) {
                structuredTexts << it.value().toString();
            }
        }
        QString combinedText = structuredTexts.join(" ");

        if (containsNonInvoiceKeyword(combinedText)) {
            for (const QString &keyword : NON_INVOICE_KEYWORDS) {
                if (combinedText.contains(keyword, Qt::CaseInsensitive)) {
                    invoice.isValidInvoice = false;
                    invoice.invalidReason = QStringLiteral("非发票文档（检测到关键词：%1）").arg(keyword);
                    debugLog(QString("Non-invoice document detected in structured data (keyword: %1), marking as invalid").arg(keyword));
                    return invoice;
                }
            }
        }
    }

    invoice.invoiceType = findText({"invoiceType", "type", "发票类型"});
    invoice.invoiceNumber = findText({"invoiceNumber", "number", "invoiceNo", "发票号码", "发票号", "票据号码"});
    debugLog(QString("Invoice type: %1, Number: %2").arg(invoice.invoiceType).arg(invoice.invoiceNumber));

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
        invoice.totalAmount = OcrParser::extractLabeledAmount(rawText, {
            QStringLiteral("价税合计"), QStringLiteral("合计"), QStringLiteral("总计"),
            QStringLiteral("应付"), QStringLiteral("实付"), QStringLiteral("小写"), QStringLiteral("金额")
        });
    }
    debugLog(QString("Total amount: %1").arg(invoice.totalAmount));

    invoice.amountWithoutTax = findNumber({
        "amountWithoutTax", "netAmount", "subtotal", "不含税金额", "金额合计"
    });
    if (invoice.amountWithoutTax == 0.0) {
        invoice.amountWithoutTax = OcrParser::extractLabeledAmount(rawText, {
            QStringLiteral("不含税"), QStringLiteral("金额合计"), QStringLiteral("净额")
        });
    }

    invoice.taxAmount = findNumber({"taxAmount", "tax", "taxTotal", "税额"});
    if (invoice.taxAmount == 0.0) {
        invoice.taxAmount = OcrParser::extractLabeledAmount(rawText, {
            QStringLiteral("税额"), QStringLiteral("税金")
        });
    }

    invoice.taxRate = findNumber({"taxRate", "rate", "税率"});
    if (invoice.taxRate == 0.0) {
        invoice.taxRate = extractTaxRateFromText(rawText);
    }

    // For API-based OCR, mark as valid if we have invoice number or total amount
    if (!invoice.invoiceNumber.isEmpty() || invoice.totalAmount > 0) {
        invoice.isValidInvoice = true;
        debugLog("Marked as valid invoice (API data with invoice number or amount)");
    } else {
        invoice.isValidInvoice = false;
        invoice.invalidReason = QStringLiteral("API返回数据中未找到发票号码或金额");
        debugLog("Marked as INVALID invoice (no invoice number or amount from API)");
    }

    // Reconcile amounts
    reconcileAmounts(invoice);

    invoice.departure = findText({"departure", "from", "出发地", "始发地"});
    invoice.destination = findText({"destination", "to", "目的地", "到达地"});
    invoice.passengerName = findText({"passengerName", "passenger", "乘客姓名"});
    invoice.stayDays = static_cast<int>(findNumber({"stayDays", "入住天数", "days"}));

    QJsonValue itemsValue = OcrParser::findValueByKeysDeep(normalizedValue, {"items", "details", "明细"});
    if (itemsValue.isArray()) {
        QJsonArray items = itemsValue.toArray();
        for (const auto &item : items) {
            if (!item.isObject()) {
                continue;
            }
            QJsonObject itemObj = item.toObject();
            InvoiceItem invItem;
            invItem.description = OcrParser::firstNonEmpty(itemObj, {"description", "name", "名称", "项目"});
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
            invItem.taxRate = parseAmount(itemObj.value("taxRate"));
            if (invItem.taxRate == 0.0) {
                invItem.taxRate = parseAmount(itemObj.value("tax"));
            }
            invItem.taxAmount = parseAmount(itemObj.value("taxAmount"));
            if (invItem.taxAmount == 0.0) {
                invItem.taxAmount = parseAmount(itemObj.value("taxFee"));
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
