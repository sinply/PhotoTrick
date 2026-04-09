#include "ItineraryRecognizer.h"
#include "../core/OcrManager.h"
#include <QtConcurrent>

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
                this, &ItineraryRecognizer::recognitionError);
    }
}

void ItineraryRecognizer::recognize(const QImage &image)
{
    if (!m_ocrManager) {
        emit recognitionError(tr("OCR管理器未设置"));
        return;
    }
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
    ItineraryData itinerary = parseItineraryData(result);
    emit recognitionFinished(itinerary);
}

ItineraryData ItineraryRecognizer::parseItineraryData(const QJsonObject &json)
{
    ItineraryData itinerary;

    // 类型
    itinerary.type = ItineraryData::typeFromString(json["type"].toString());
    itinerary.typeString = json["type"].toString();

    // 乘客信息
    itinerary.passengerName = json["passengerName"].toString();

    // 行程信息
    itinerary.departure = json["departure"].toString();
    itinerary.destination = json["destination"].toString();

    QString depTimeStr = json["departureTime"].toString();
    if (!depTimeStr.isEmpty()) {
        itinerary.departureTime = QDateTime::fromString(depTimeStr, "yyyy-MM-dd HH:mm");
    }

    QString arrTimeStr = json["arrivalTime"].toString();
    if (!arrTimeStr.isEmpty()) {
        itinerary.arrivalTime = QDateTime::fromString(arrTimeStr, "yyyy-MM-dd HH:mm");
    }

    // 交通工具信息
    itinerary.flightTrainNo = json["flightTrainNo"].toString();
    if (itinerary.flightTrainNo.isEmpty()) {
        itinerary.flightTrainNo = json["flightNumber"].toString();
    }
    if (itinerary.flightTrainNo.isEmpty()) {
        itinerary.flightTrainNo = json["trainNumber"].toString();
    }
    itinerary.seatClass = json["seatClass"].toString();
    itinerary.seatNumber = json["seatNumber"].toString();

    // 金额
    itinerary.price = json["price"].toDouble(0.0);
    if (itinerary.price == 0.0) {
        itinerary.price = json["totalAmount"].toDouble(0.0);
    }
    itinerary.taxAmount = json["taxAmount"].toDouble(0.0);

    return itinerary;
}
