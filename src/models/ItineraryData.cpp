#include "ItineraryData.h"

QJsonObject ItineraryData::toJson() const
{
    QJsonObject obj;
    obj["type"] = static_cast<int>(type);
    obj["typeString"] = typeString;
    obj["passengerName"] = passengerName;
    obj["departure"] = departure;
    obj["destination"] = destination;

    if (departureTime.isValid()) {
        obj["departureTime"] = departureTime.toString(Qt::ISODate);
    }
    if (arrivalTime.isValid()) {
        obj["arrivalTime"] = arrivalTime.toString(Qt::ISODate);
    }

    obj["flightTrainNo"] = flightTrainNo;
    obj["seatClass"] = seatClass;
    obj["seatNumber"] = seatNumber;
    obj["price"] = price;
    obj["taxAmount"] = taxAmount;
    obj["currency"] = currency;
    obj["relatedInvoiceId"] = relatedInvoiceId;
    obj["sourceFile"] = sourceFile;
    obj["confidence"] = confidence;

    return obj;
}

ItineraryData ItineraryData::fromJson(const QJsonObject &json)
{
    ItineraryData data;
    data.type = static_cast<Type>(json["type"].toInt());
    data.typeString = json["typeString"].toString();
    data.passengerName = json["passengerName"].toString();
    data.departure = json["departure"].toString();
    data.destination = json["destination"].toString();

    if (json.contains("departureTime")) {
        data.departureTime = QDateTime::fromString(json["departureTime"].toString(), Qt::ISODate);
    }
    if (json.contains("arrivalTime")) {
        data.arrivalTime = QDateTime::fromString(json["arrivalTime"].toString(), Qt::ISODate);
    }

    data.flightTrainNo = json["flightTrainNo"].toString();
    data.seatClass = json["seatClass"].toString();
    data.seatNumber = json["seatNumber"].toString();
    data.price = json["price"].toDouble();
    data.taxAmount = json["taxAmount"].toDouble();
    data.currency = json["currency"].toString("CNY");
    data.relatedInvoiceId = json["relatedInvoiceId"].toString();
    data.sourceFile = json["sourceFile"].toString();
    data.confidence = json["confidence"].toDouble();

    return data;
}

QString ItineraryData::typeToString() const
{
    switch (type) {
    case Flight: return QStringLiteral("机票");
    case Train: return QStringLiteral("火车票");
    case Bus: return QStringLiteral("汽车票");
    default: return QStringLiteral("其他");
    }
}

ItineraryData::Type ItineraryData::typeFromString(const QString &str)
{
    if (str == QStringLiteral("机票") || str == "Flight" || str.contains(QStringLiteral("航空"))) {
        return Flight;
    } else if (str == QStringLiteral("火车票") || str == "Train" || str.contains(QStringLiteral("火车")) || str.contains(QStringLiteral("高铁"))) {
        return Train;
    } else if (str == QStringLiteral("汽车票") || str == "Bus" || str.contains(QStringLiteral("汽车"))) {
        return Bus;
    }
    return Other;
}
