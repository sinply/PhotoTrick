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
    obj["fuelSurcharge"] = fuelSurcharge;
    obj["airportTax"] = airportTax;
    obj["insurance"] = insurance;
    obj["totalAmount"] = totalAmount;
    obj["currency"] = currency;
    obj["relatedInvoiceId"] = relatedInvoiceId;
    obj["sourceFile"] = sourceFile;
    obj["confidence"] = confidence;
    obj["isValidItinerary"] = isValidItinerary;
    obj["invalidReason"] = invalidReason;

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
    data.fuelSurcharge = json["fuelSurcharge"].toDouble();
    data.airportTax = json["airportTax"].toDouble();
    data.insurance = json["insurance"].toDouble();
    data.totalAmount = json["totalAmount"].toDouble();
    data.currency = json["currency"].toString("CNY");
    data.relatedInvoiceId = json["relatedInvoiceId"].toString();
    data.sourceFile = json["sourceFile"].toString();
    data.confidence = json["confidence"].toDouble();
    data.isValidItinerary = json["isValidItinerary"].toBool();
    data.invalidReason = json["invalidReason"].toString();

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

void ItineraryData::validate()
{
    // A valid itinerary needs at least one transport-specific identifier
    bool hasTransportId = !flightTrainNo.trimmed().isEmpty();
    bool hasRoute = !departure.trimmed().isEmpty() && !destination.trimmed().isEmpty();
    bool hasTimeInfo = departureTime.isValid() || arrivalTime.isValid();
    bool hasPassenger = !passengerName.trimmed().isEmpty();
    bool hasPrice = price > 0.0;

    // Minimum: transport ID OR route + (time or passenger)
    // Price alone is NOT sufficient (invoices also have amounts)
    bool hasCoreData = hasTransportId || (hasRoute && (hasTimeInfo || hasPassenger));

    if (!hasCoreData) {
        isValidItinerary = false;
        invalidReason = QStringLiteral("缺少行程单核心信息（航班/车次、路线+时间/乘客）");
        return;
    }

    // Check for strong invoice markers that indicate this is NOT an itinerary
    // These keywords are exclusive to formal invoices
    static const QStringList strongInvoiceKeywords = {
        QStringLiteral("价税合计"), QStringLiteral("发票代码"),
        QStringLiteral("税率/征收率"), QStringLiteral("开票人"),
        QStringLiteral("复核"), QStringLiteral("收款人"),
        QStringLiteral("销售方信息"), QStringLiteral("购买方信息"),
        QStringLiteral("税务局")
    };

    // Strong invoice keywords override itinerary classification
    // unless we also have strong itinerary-specific markers (登机牌, 行程单, 客票级别 etc.)
    static const QStringList strongItineraryKeywords = {
        QStringLiteral("登机牌"), QStringLiteral("BOARDING PASS"),
        QStringLiteral("行程单"), QStringLiteral("ITINERARY"),
        QStringLiteral("电子客票"), QStringLiteral("E-TICKET"),
        QStringLiteral("客票级别"), QStringLiteral("客票号码"),
        QStringLiteral("印刷序号"), QStringLiteral("民航发展基金"),
        QStringLiteral("机建费"), QStringLiteral("燃油附加费"),
        QStringLiteral("车厢"), QStringLiteral("检票口"),
        QStringLiteral("客票号")
    };

    int invoiceCount = 0;
    int itineraryCount = 0;
    // Check sourceFile + existing typeString for itinerary markers (already set during parsing)
    QString checkText = typeString + " " + flightTrainNo;
    if (type == Flight) itineraryCount += 2;
    if (type == Train) itineraryCount += 2;

    for (const QString &kw : strongInvoiceKeywords) {
        if (checkText.contains(kw, Qt::CaseInsensitive)) invoiceCount++;
    }
    for (const QString &kw : strongItineraryKeywords) {
        if (checkText.contains(kw, Qt::CaseInsensitive)) itineraryCount++;
    }

    // If strong invoice markers dominate, this is likely not an itinerary
    if (invoiceCount >= 2 && itineraryCount == 0) {
        isValidItinerary = false;
        invalidReason = QStringLiteral("检测到发票特征，非行程单文档");
        return;
    }

    isValidItinerary = true;
    invalidReason.clear();
}
