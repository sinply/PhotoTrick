#ifndef ITINERARYDATA_H
#define ITINERARYDATA_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>

struct ItineraryData {
    enum Type {
        Flight,
        Train,
        Bus,
        Other
    };

    // Basic info
    Type type = Other;
    QString typeString;         // 类型字符串

    // Passenger
    QString passengerName;      // 乘客姓名

    // Route
    QString departure;          // 出发地
    QString destination;        // 目的地
    QDateTime departureTime;    // 出发时间
    QDateTime arrivalTime;      // 到达时间

    // Transport info
    QString flightTrainNo;      // 航班号/车次
    QString seatClass;          // 舱位/座位等级
    QString seatNumber;         // 座位号

    // Price
    double price = 0.0;         // 票价
    double taxAmount = 0.0;     // 税额
    double fuelSurcharge = 0.0; // 燃油附加费
    double airportTax = 0.0;    // 机建费
    double insurance = 0.0;     // 保险费
    double totalAmount = 0.0;   // 总金额
    QString currency = "CNY";

    // Relations
    QString relatedInvoiceId;   // 关联发票ID

    // Source
    QString sourceFile;

    // Validation
    bool isValidItinerary = false;
    QString invalidReason;

    // Confidence
    double confidence = 0.0;

    // Serialization
    QJsonObject toJson() const;
    static ItineraryData fromJson(const QJsonObject &json);

    // Helper
    QString typeToString() const;
    static Type typeFromString(const QString &str);
    void validate();
};

#endif // ITINERARYDATA_H
