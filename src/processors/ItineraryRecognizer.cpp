#include "ItineraryRecognizer.h"
#include "../core/OcrManager.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QRegularExpression>
#include <QStringList>
#include <QtConcurrent>
#include <functional>
#include <algorithm>

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
    return !trimmed.isEmpty()
        && trimmed.compare(QStringLiteral("null"), Qt::CaseInsensitive) != 0
        && trimmed.compare(QStringLiteral("none"), Qt::CaseInsensitive) != 0
        && trimmed != QStringLiteral("-")
        && trimmed != QStringLiteral("--");
}

QJsonValue findValueByKeysDeep(const QJsonValue &root, const QStringList &keys)
{
    QStringList normalizedTargets;
    for (const QString &k : keys) {
        const QString nk = normalizeKey(k);
        if (!nk.isEmpty()) {
            normalizedTargets.append(nk);
        }
    }

    // Two-pass: exact match first, then substring match
    std::function<QJsonValue(const QJsonValue &, bool)> walk = [&](const QJsonValue &node, bool exactOnly) -> QJsonValue {
        if (node.isObject()) {
            const QJsonObject obj = node.toObject();

            // First: exact matches
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                const QString keyNorm = normalizeKey(it.key());
                for (const QString &target : normalizedTargets) {
                    if (keyNorm == target) {
                        if (it.value().isString() && isMeaningfulString(it.value().toString())) {
                            return it.value();
                        }
                        if (it.value().isDouble()) {
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
                        if (it.value().isString() && isMeaningfulString(it.value().toString())) {
                            return it.value();
                        }
                        if (it.value().isDouble()) {
                            return it.value();
                        }
                    }
                }
            }

            // Recurse
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                if (it.value().isObject() || it.value().isArray()) {
                    const QJsonValue nested = walk(it.value(), exactOnly);
                    if (!nested.isUndefined()) {
                        return nested;
                    }
                }
            }
        } else if (node.isArray()) {
            const QJsonArray arr = node.toArray();
            for (const QJsonValue &item : arr) {
                if (item.isObject() || item.isArray()) {
                    const QJsonValue nested = walk(item, exactOnly);
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
        const QJsonValue val = json.value(key);
        if (val.isString() && !val.toString().trimmed().isEmpty()) {
            chunks << val.toString().trimmed();
        }
    };

    appendFromKey(QStringLiteral("text"));
    appendFromKey(QStringLiteral("raw_result"));
    appendFromKey(QStringLiteral("content"));

    if (chunks.isEmpty() && json.contains(QStringLiteral("data")) && json.value(QStringLiteral("data")).isObject()) {
        chunks << extractRawText(json.value(QStringLiteral("data")).toObject());
    }

    return chunks.join('\n').trimmed();
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

double parseNumber(const QJsonValue &value)
{
    if (value.isDouble()) {
        return value.toDouble();
    }
    QString str = value.toString().trimmed();
    if (str.isEmpty()) {
        return 0.0;
    }
    str.remove(QChar(0xFFE5));
    str.remove(QChar(0x00A5));
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

double extractLabeledAmount(const QString &rawText, const QStringList &labels)
{
    struct AmountCandidate {
        double value = 0.0;
        int score = 0;
        QString context;
    };

    const QStringList negativeLabels = {
        QStringLiteral("税率"), QStringLiteral("%"), QStringLiteral("百分比"),
        QStringLiteral("编号"), QStringLiteral("代码"), QStringLiteral("号码"),
        QStringLiteral("数量"), QStringLiteral("单价"), QStringLiteral("件"),
        QStringLiteral("序号"), QStringLiteral("电话"), QStringLiteral("手机")
    };

    const QStringList lines = rawText.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);
    QList<AmountCandidate> candidates;

    for (const QString &lineRaw : lines) {
        const QString line = lineRaw.trimmed();
        if (line.isEmpty()) continue;

        int baseScore = 0;
        for (const QString &label : labels) {
            if (line.contains(label, Qt::CaseInsensitive)) {
                baseScore += line.startsWith(label, Qt::CaseInsensitive) ? 10 : 5;
            }
        }
        if (baseScore == 0) continue;

        for (const QString &negLabel : negativeLabels) {
            if (line.contains(negLabel, Qt::CaseInsensitive)) baseScore -= 8;
        }

        QRegularExpression re(QStringLiteral(R"(([-+]?\d{1,3}(?:,\d{3})*(?:\.\d+)?|[-+]?\d+(?:\.\d+)?))"));
        auto it = re.globalMatch(line);

        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            const QString numStr = match.captured(1);
            bool ok = false;
            const double value = QString(numStr).remove(',').toDouble(&ok);
            if (!ok || value <= 0.0) continue;

            AmountCandidate candidate;
            candidate.value = value;
            candidate.context = line;
            candidate.score = baseScore;

            const int numPos = match.capturedStart(1);
            const QString beforeNum = line.left(numPos);
            const QString afterNum = line.mid(numPos + numStr.length());

            if (beforeNum.contains(QChar(0xFFE5)) || beforeNum.contains(QChar(0x00A5)) ||
                beforeNum.contains('$') || beforeNum.contains(QStringLiteral("¥")) ||
                beforeNum.contains(QStringLiteral("元"))) candidate.score += 15;
            if (afterNum.contains(QChar(0xFFE5)) || afterNum.contains(QChar(0x00A5)) ||
                afterNum.contains(QStringLiteral("元"))) candidate.score += 10;

            if (beforeNum.contains('%') || afterNum.contains('%') ||
                beforeNum.contains(QStringLiteral("％")) || afterNum.contains(QStringLiteral("％"))) {
                candidate.score -= 20;
            }

            if (value >= 100) candidate.score += 3;
            if (value >= 1000) candidate.score += 2;
            if (value < 1.0 && value > 0.0) candidate.score -= 10;

            candidates.append(candidate);
        }
    }

    if (candidates.isEmpty()) return 0.0;

    std::sort(candidates.begin(), candidates.end(), [](const AmountCandidate &a, const AmountCandidate &b) {
        return a.score > b.score;
    });
    return candidates.first().value;
}

QJsonObject choosePrimaryObject(const QJsonObject &json)
{
    if (json.contains(QStringLiteral("data")) && json.value(QStringLiteral("data")).isObject()) {
        return json.value(QStringLiteral("data")).toObject();
    }
    if (json.contains(QStringLiteral("result")) && json.value(QStringLiteral("result")).isObject()) {
        return json.value(QStringLiteral("result")).toObject();
    }
    return json;
}
}

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

ItineraryData ItineraryRecognizer::parseItineraryData(const QJsonObject &json)
{
    ItineraryData itinerary;
    const QString rawText = extractRawText(json);

    qDebug() << "ItineraryRecognizer: Raw OCR text:" << rawText.left(500);

    QJsonObject normalized = choosePrimaryObject(json);
    if (normalized.contains("text") && normalized["text"].isString()) {
        QJsonObject parsed = parseJsonObjectFromText(normalized["text"].toString());
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

        // Extract price with candidate scoring
        itinerary.price = extractLabeledAmount(rawText, {
            QStringLiteral("票价"), QStringLiteral("票款"), QStringLiteral("金额"),
            QStringLiteral("应付"), QStringLiteral("实付"), QStringLiteral("价格")
        });
        qDebug() << "ItineraryRecognizer: Extracted price:" << itinerary.price;

        // Extract tax/fees
        itinerary.taxAmount = extractLabeledAmount(rawText, {
            QStringLiteral("税额"), QStringLiteral("机建"), QStringLiteral("燃油"), QStringLiteral("税费")
        });

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
        double value = parseNumber(findValueByKeysDeep(normalizedValue, keys));
        if (value > 0.0) {
            return value;
        }
        for (const QString &key : keys) {
            if (normalized.contains(key)) {
                value = parseNumber(normalized.value(key));
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
        itinerary.price = extractLabeledAmount(rawText, {
            QStringLiteral("票价"), QStringLiteral("票款"), QStringLiteral("金额"),
            QStringLiteral("应付"), QStringLiteral("实付")
        });
    }

    itinerary.taxAmount = findNumber({"taxAmount", "tax", "税额", "机建", "燃油"});
    if (itinerary.taxAmount == 0.0) {
        itinerary.taxAmount = extractLabeledAmount(rawText, {
            QStringLiteral("税额"), QStringLiteral("机建"), QStringLiteral("燃油")
        });
    }

    if (itinerary.type == ItineraryData::Other) {
        const QString hint = rawText + "\n" + itinerary.flightTrainNo + "\n" + itinerary.typeString;
        if (hint.contains(QStringLiteral("航班")) || hint.contains(QStringLiteral("MU")) || hint.contains(QStringLiteral("CA"))) {
            itinerary.type = ItineraryData::Flight;
        } else if (hint.contains(QStringLiteral("车次")) || hint.contains(QStringLiteral("高铁")) || hint.contains(QStringLiteral("G"))) {
            itinerary.type = ItineraryData::Train;
        }
    }

    return itinerary;
}
