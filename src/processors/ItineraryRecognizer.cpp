#include "ItineraryRecognizer.h"
#include "../core/OcrManager.h"
#include "../utils/OcrParser.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QRegularExpression>
#include <QStringList>
#include <QtConcurrent>
#include <functional>
#include <algorithm>


const QString ItineraryRecognizer::ITINERARY_PROMPT = R"(
请分析这张行程单图片，提取以下信息并以JSON格式返回：

{
    "type": "行程单类型（机票/火车票/汽车票/出租车/网约车/其他）",
    "passengerName": "乘客姓名",
    "departure": "出发地",
    "destination": "目的地",
    "departureTime": "出发时间（YYYY-MM-DD HH:mm格式）",
    "arrivalTime": "到达时间（YYYY-MM-DD HH:mm格式）",
    "flightTrainNo": "航班号/车次",
    "seatClass": "舱位/座位等级",
    "seatNumber": "座位号",
    "price": "票价（数字）",
    "taxAmount": "税额（数字）"
}

请确保返回有效的JSON格式。如果某字段无法识别，请填null。
)";

ItineraryRecognizer::ItineraryRecognizer(QObject *parent)
    : QObject(parent)
    , m_ocrManager(nullptr)
{
}

void ItineraryRecognizer::setOcrManager(OcrManager *manager)
{
    if (m_ocrManager) {
        disconnect(m_ocrManager, nullptr, this, nullptr);
    }
    m_ocrManager = manager;
    if (m_ocrManager) {
        connect(m_ocrManager, &OcrManager::recognitionFinished,
                this, &ItineraryRecognizer::onOcrFinished);
        connect(m_ocrManager, &OcrManager::recognitionError,
                this, &ItineraryRecognizer::onOcrError);
    }
}

void ItineraryRecognizer::recognize(const QImage &image)
{
    if (!m_ocrManager) {
        emit recognitionError(tr("OCR管理器未设置"));
        return;
    }
    m_isRecognizing = true;
    m_ocrManager->recognizeImage(image, ITINERARY_PROMPT);
}

void ItineraryRecognizer::recognizeAsync(const QImage &image)
{
    if (!m_ocrManager) {
        emit recognitionError(tr("OCR管理器未设置"));
        return;
    }

    QtConcurrent::run([this, image]() {
        recognize(image);
    });
}

void ItineraryRecognizer::onOcrFinished(const QJsonObject &result)
{
    if (!m_isRecognizing) {
        return;
    }
    m_isRecognizing = false;
    emit rawOcrReceived(result);

    ItineraryData itinerary = parseItineraryData(result);
    emit recognitionFinished(itinerary);
}

void ItineraryRecognizer::onOcrError(const QString &error)
{
    if (!m_isRecognizing) {
        return;
    }
    m_isRecognizing = false;
    emit recognitionError(error);
}

namespace {
QDateTime parseChineseDateTime(const QString &raw)
{
    QString s = raw;
    s.replace(QStringLiteral("年"), QStringLiteral("-"))
     .replace(QStringLiteral("月"), QStringLiteral("-"))
     .replace(QStringLiteral("日"), QString())
     .replace(QStringLiteral("时"), QStringLiteral(":"))
     .replace(QStringLiteral("分"), QString());
    QDateTime dt = QDateTime::fromString(s.trimmed(), "yyyy-M-d H:mm");
    if (!dt.isValid()) {
        dt = QDateTime::fromString(s.trimmed(), "yyyy-MM-dd HH:mm");
    }
    return dt;
}
} // anonymous namespace

ItineraryData ItineraryRecognizer::parseItineraryData(const QJsonObject &json)
{
    ItineraryData itinerary;
    const QString rawText = OcrParser::extractRawText(json);

    qDebug() << "ItineraryRecognizer: Raw OCR text:" << rawText.left(500);

    QJsonObject normalized = OcrParser::choosePrimaryObject(json);
    if (normalized.contains("text") && normalized["text"].isString()) {
        QJsonObject parsed = OcrParser::parseJsonObjectFromText(normalized["text"].toString());
        if (!parsed.isEmpty()) {
            normalized = parsed;
        }
    }

    const QJsonValue normalizedValue(normalized);

    // Check if we have structured JSON data or just raw OCR text
    // Local PaddleOCR returns {success, text, boxes} - this is NOT structured itinerary data
    bool hasStructuredData = false;
    if (!normalized.isEmpty()) {
        QStringList itineraryFields = {
            QStringLiteral("type"), QStringLiteral("passengerName"), QStringLiteral("departure"),
            QStringLiteral("destination"), QStringLiteral("flightTrainNo"), QStringLiteral("price"),
            QStringLiteral("departureTime"), QStringLiteral("arrivalTime")
        };
        int matchCount = 0;
        for (const QString &field : itineraryFields) {
            if (normalized.contains(field)) {
                matchCount++;
            }
        }
        hasStructuredData = matchCount >= 2;
    }

    // For local OCR (no structured JSON), parse from raw text
    if (!hasStructuredData && !rawText.isEmpty()) {
        qDebug() << "ItineraryRecognizer: Using raw text parsing for local OCR";

        // Extract flight/train number
        QRegularExpression flightRe(QStringLiteral(R"((?:航班号|车次|班次)[：:\s]*([A-Z0-9]+))"));
        QRegularExpressionMatch match = flightRe.match(rawText);
        if (match.hasMatch()) {
            itinerary.flightTrainNo = match.captured(1);
        }

        // Also try to find flight numbers like CA1234, MU5678 in text
        QRegularExpression flightNumRe(QStringLiteral(R"(\b([A-Z]{2}\d{3,4})\b)"));
        match = flightNumRe.match(rawText);
        if (match.hasMatch()) {
            itinerary.flightTrainNo = match.captured(1);
        }

        // Extract passenger name
        QRegularExpression passengerRe(QStringLiteral(R"((?:乘客|旅客|姓名)[：:\s]*([^\n\r]+))"));
        match = passengerRe.match(rawText);
        if (match.hasMatch()) {
            itinerary.passengerName = match.captured(1).trimmed();
        }

        // Extract departure and destination
        QRegularExpression routeRe(QStringLiteral(R"((?:出发|始发|from)[：:\s]*([^\n\r]+?)(?:\s*[-→>]\s*|\s+(?:到达|to)[：:\s]*)([^\n\r]+))"));
        match = routeRe.match(rawText);
        if (match.hasMatch()) {
            itinerary.departure = match.captured(1).trimmed();
            itinerary.destination = match.captured(2).trimmed();
        }

        // Extract departure time
        QRegularExpression depTimeRe(QStringLiteral(
            R"((?:出发时间|起飞时间|开车时间|出发|起飞)[：:\s]*(\d{4}[-/年]\d{1,2}[-/月]\d{1,2}日?\s*\d{1,2}[：:时]\d{1,2}(?:分)?))"));
        match = depTimeRe.match(rawText);
        if (match.hasMatch()) {
            itinerary.departureTime = parseChineseDateTime(match.captured(1));
        }
        // Also try standalone date+time patterns
        if (!itinerary.departureTime.isValid()) {
            QRegularExpression dateTimeRe(QStringLiteral(
                R"((\d{4})[-/年](\d{1,2})[-/月](\d{1,2})日?\s*(\d{1,2})[：:时](\d{1,2}))"));
            match = dateTimeRe.match(rawText);
            if (match.hasMatch()) {
                itinerary.departureTime = QDateTime(
                    QDate(match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt()),
                    QTime(match.captured(4).toInt(), match.captured(5).toInt()));
            }
        }

        // Extract arrival time
        QRegularExpression arrTimeRe(QStringLiteral(
            R"((?:到达时间|降落时间|到达|降落)[：:\s]*(\d{4}[-/年]\d{1,2}[-/月]\d{1,2}日?\s*\d{1,2}[：:时]\d{1,2}(?:分)?))"));
        match = arrTimeRe.match(rawText);
        if (match.hasMatch()) {
            itinerary.arrivalTime = parseChineseDateTime(match.captured(1));
        }

        // Extract date alone (for train tickets that may only show date)
        if (!itinerary.departureTime.isValid()) {
            QRegularExpression dateRe(QStringLiteral(
                R"((\d{4})[-/年](\d{1,2})[-/月](\d{1,2})日?)"));
            match = dateRe.match(rawText);
            if (match.hasMatch()) {
                itinerary.departureTime = QDateTime(
                    QDate(match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt()),
                    QTime(0, 0));
            }
        }

        // Extract price with candidate scoring
        itinerary.price = OcrParser::extractLabeledAmount(rawText, {
            QStringLiteral("票价"), QStringLiteral("票款"), QStringLiteral("金额"),
            QStringLiteral("应付"), QStringLiteral("实付"), QStringLiteral("价格")
        });
        qDebug() << "ItineraryRecognizer: Extracted price:" << itinerary.price;

        // Extract tax/fees
        itinerary.taxAmount = OcrParser::extractLabeledAmount(rawText, {
            QStringLiteral("税额"), QStringLiteral("税费")
        });

        // Extract fare breakdown for flights
        itinerary.fuelSurcharge = OcrParser::extractLabeledAmount(rawText, {
            QStringLiteral("燃油附加费"), QStringLiteral("燃油费"), QStringLiteral("燃油")
        });
        itinerary.airportTax = OcrParser::extractLabeledAmount(rawText, {
            QStringLiteral("民航发展基金"), QStringLiteral("机建费"), QStringLiteral("机场建设费"), QStringLiteral("机建")
        });
        itinerary.insurance = OcrParser::extractLabeledAmount(rawText, {
            QStringLiteral("保险费"), QStringLiteral("保险")
        });

        // Compute total if not explicitly labeled
        itinerary.totalAmount = OcrParser::extractLabeledAmount(rawText, {
            QStringLiteral("合计"), QStringLiteral("总计"), QStringLiteral("总额"), QStringLiteral("总金额")
        });
        if (itinerary.totalAmount == 0.0 && itinerary.price > 0.0) {
            itinerary.totalAmount = itinerary.price + itinerary.taxAmount
                + itinerary.fuelSurcharge + itinerary.airportTax + itinerary.insurance;
        }

        // Determine type from content
        if (rawText.contains(QStringLiteral("航班")) || rawText.contains(QStringLiteral("机票"))
            || rawText.contains(QStringLiteral("CA")) || rawText.contains(QStringLiteral("MU"))
            || rawText.contains(QStringLiteral("CZ")) || rawText.contains(QStringLiteral("HU"))) {
            itinerary.type = ItineraryData::Flight;
            itinerary.typeString = QStringLiteral("机票");
        } else if (rawText.contains(QStringLiteral("火车")) || rawText.contains(QStringLiteral("高铁"))
                   || rawText.contains(QStringLiteral("车次"))) {
            itinerary.type = ItineraryData::Train;
            itinerary.typeString = QStringLiteral("火车票");
        } else if (rawText.contains(QStringLiteral("出租")) || rawText.contains(QStringLiteral("网约车"))) {
            itinerary.type = ItineraryData::Other;
            itinerary.typeString = QStringLiteral("出租车");
        }

        qDebug() << "ItineraryRecognizer: Parsed from raw text - FlightNo:" << itinerary.flightTrainNo
                 << "Price:" << itinerary.price << "Type:" << itinerary.typeString;

        return itinerary;
    }

    // Structured JSON parsing (for API-based OCR)
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
        double value = OcrParser::parseNumber(OcrParser::findValueByKeysDeep(normalizedValue, keys));
        if (value > 0.0) {
            return value;
        }
        for (const QString &key : keys) {
            if (normalized.contains(key)) {
                value = OcrParser::parseNumber(normalized.value(key));
                if (value > 0.0) {
                    return value;
                }
            }
        }
        return 0.0;
    };

    itinerary.typeString = findText({"type", "itineraryType", "行程单类型", "票据类型"});
    itinerary.type = ItineraryData::typeFromString(itinerary.typeString);

    itinerary.passengerName = findText({"passengerName", "passenger", "name", "乘客姓名"});

    itinerary.departure = findText({"departure", "from", "origin", "出发地", "始发站", "出发站"});
    itinerary.destination = findText({"destination", "to", "arrival", "目的地", "到达站", "终点站"});

    QString depTimeStr = findText({"departureTime", "startTime", "出发时间"});
    if (!depTimeStr.isEmpty()) {
        itinerary.departureTime = QDateTime::fromString(depTimeStr, "yyyy-MM-dd HH:mm");
        if (!itinerary.departureTime.isValid()) {
            itinerary.departureTime = QDateTime::fromString(depTimeStr, Qt::ISODate);
        }
    }

    QString arrTimeStr = findText({"arrivalTime", "endTime", "到达时间"});
    if (!arrTimeStr.isEmpty()) {
        itinerary.arrivalTime = QDateTime::fromString(arrTimeStr, "yyyy-MM-dd HH:mm");
        if (!itinerary.arrivalTime.isValid()) {
            itinerary.arrivalTime = QDateTime::fromString(arrTimeStr, Qt::ISODate);
        }
    }

    itinerary.flightTrainNo = findText({"flightTrainNo", "flightNumber", "trainNumber", "flightNo", "ticketNo", "航班号", "车次"});
    itinerary.seatClass = findText({"seatClass", "cabinClass", "舱位", "座位等级", "席别"});
    itinerary.seatNumber = findText({"seatNumber", "seatNo", "座位号", "座号"});

    itinerary.price = findNumber({"price", "amount", "fare", "totalAmount", "票价", "票款", "金额"});
    if (itinerary.price == 0.0) {
        itinerary.price = OcrParser::extractLabeledAmount(rawText, {
            QStringLiteral("票价"), QStringLiteral("票款"), QStringLiteral("金额"),
            QStringLiteral("应付"), QStringLiteral("实付")
        });
    }

    itinerary.taxAmount = findNumber({"taxAmount", "tax", "税额"});
    if (itinerary.taxAmount == 0.0) {
        itinerary.taxAmount = OcrParser::extractLabeledAmount(rawText, {
            QStringLiteral("税额"), QStringLiteral("税费")
        });
    }

    itinerary.fuelSurcharge = findNumber({"fuelSurcharge", "fuelFee", "燃油附加费", "燃油费"});
    if (itinerary.fuelSurcharge == 0.0) {
        itinerary.fuelSurcharge = OcrParser::extractLabeledAmount(rawText, {
            QStringLiteral("燃油附加费"), QStringLiteral("燃油费"), QStringLiteral("燃油")
        });
    }

    itinerary.airportTax = findNumber({"airportTax", "airportFee", "constructionFee", "机建费", "机场建设费", "民航发展基金"});
    if (itinerary.airportTax == 0.0) {
        itinerary.airportTax = OcrParser::extractLabeledAmount(rawText, {
            QStringLiteral("民航发展基金"), QStringLiteral("机建费"), QStringLiteral("机场建设费"), QStringLiteral("机建")
        });
    }

    itinerary.insurance = findNumber({"insurance", "insuranceFee", "保险费"});
    if (itinerary.insurance == 0.0) {
        itinerary.insurance = OcrParser::extractLabeledAmount(rawText, {
            QStringLiteral("保险费"), QStringLiteral("保险")
        });
    }

    itinerary.totalAmount = findNumber({"totalAmount", "total", "合计", "总计"});
    if (itinerary.totalAmount == 0.0) {
        itinerary.totalAmount = OcrParser::extractLabeledAmount(rawText, {
            QStringLiteral("合计"), QStringLiteral("总计"), QStringLiteral("总额"), QStringLiteral("总金额")
        });
    }
    if (itinerary.totalAmount == 0.0 && itinerary.price > 0.0) {
        itinerary.totalAmount = itinerary.price + itinerary.taxAmount
            + itinerary.fuelSurcharge + itinerary.airportTax + itinerary.insurance;
    }

    if (itinerary.type == ItineraryData::Other) {
        const QString hint = rawText + "\n" + itinerary.flightTrainNo + "\n" + itinerary.typeString;
        if (hint.contains(QStringLiteral("航班")) || hint.contains(QStringLiteral("MU")) || hint.contains(QStringLiteral("CA"))) {
            itinerary.type = ItineraryData::Flight;
        } else if (hint.contains(QStringLiteral("车次")) || hint.contains(QStringLiteral("高铁")) || hint.contains(QStringLiteral("G"))) {
            itinerary.type = ItineraryData::Train;
        }
    }

    itinerary.validate();
    return itinerary;
}
