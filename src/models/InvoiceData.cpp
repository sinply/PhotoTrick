#include "InvoiceData.h"
#include <QJsonArray>

QJsonObject InvoiceItem::toJson() const
{
    QJsonObject obj;
    obj["description"] = description;
    obj["quantity"] = quantity;
    obj["unitPrice"] = unitPrice;
    obj["amount"] = amount;
    obj["taxRate"] = taxRate;
    obj["taxAmount"] = taxAmount;
    return obj;
}

InvoiceItem InvoiceItem::fromJson(const QJsonObject &json)
{
    InvoiceItem item;
    item.description = json["description"].toString();
    item.quantity = json["quantity"].toDouble();
    item.unitPrice = json["unitPrice"].toDouble();
    item.amount = json["amount"].toDouble();
    item.taxRate = json["taxRate"].toDouble();
    item.taxAmount = json["taxAmount"].toDouble();
    return item;
}

QJsonObject InvoiceData::toJson() const
{
    QJsonObject obj;
    obj["invoiceType"] = invoiceType;
    obj["category"] = static_cast<int>(category);
    obj["invoiceNumber"] = invoiceNumber;
    obj["invoiceDate"] = invoiceDate.toString("yyyy-MM-dd");
    obj["totalAmount"] = totalAmount;
    obj["amountWithoutTax"] = amountWithoutTax;
    obj["taxAmount"] = taxAmount;
    obj["taxRate"] = taxRate;
    obj["currency"] = currency;
    obj["sellerName"] = sellerName;
    obj["sellerTaxId"] = sellerTaxId;
    obj["buyerName"] = buyerName;
    obj["buyerTaxId"] = buyerTaxId;

    if (!departure.isEmpty()) {
        obj["departure"] = departure;
    }
    if (!destination.isEmpty()) {
        obj["destination"] = destination;
    }
    if (!passengerName.isEmpty()) {
        obj["passengerName"] = passengerName;
    }
    if (stayDays > 0) {
        obj["stayDays"] = stayDays;
    }

    if (!items.isEmpty()) {
        QJsonArray itemsArray;
        for (const auto &item : items) {
            itemsArray.append(item.toJson());
        }
        obj["items"] = itemsArray;
    }

    obj["confidence"] = confidence;
    obj["sourceFile"] = sourceFile;

    return obj;
}

InvoiceData InvoiceData::fromJson(const QJsonObject &json)
{
    InvoiceData data;
    data.invoiceType = json["invoiceType"].toString();
    data.category = static_cast<Category>(json["category"].toInt());
    data.invoiceNumber = json["invoiceNumber"].toString();
    data.invoiceDate = QDate::fromString(json["invoiceDate"].toString(), "yyyy-MM-dd");
    data.totalAmount = json["totalAmount"].toDouble();
    data.amountWithoutTax = json["amountWithoutTax"].toDouble();
    data.taxAmount = json["taxAmount"].toDouble();
    data.taxRate = json["taxRate"].toDouble();
    data.currency = json["currency"].toString("CNY");
    data.sellerName = json["sellerName"].toString();
    data.sellerTaxId = json["sellerTaxId"].toString();
    data.buyerName = json["buyerName"].toString();
    data.buyerTaxId = json["buyerTaxId"].toString();
    data.departure = json["departure"].toString();
    data.destination = json["destination"].toString();
    data.passengerName = json["passengerName"].toString();
    data.stayDays = json["stayDays"].toInt();
    data.confidence = json["confidence"].toDouble();
    data.sourceFile = json["sourceFile"].toString();

    if (json.contains("items")) {
        QJsonArray itemsArray = json["items"].toArray();
        for (const auto &item : itemsArray) {
            data.items.append(InvoiceItem::fromJson(item.toObject()));
        }
    }

    return data;
}

QString InvoiceData::categoryString() const
{
    switch (category) {
    case Transportation: return QStringLiteral("交通");
    case Accommodation: return QStringLiteral("住宿");
    case Dining: return QStringLiteral("餐饮");
    default: return QStringLiteral("其他");
    }
}

InvoiceData::Category InvoiceData::categoryFromString(const QString &str)
{
    if (str == QStringLiteral("交通") || str == "Transportation") {
        return Transportation;
    } else if (str == QStringLiteral("住宿") || str == "Accommodation") {
        return Accommodation;
    } else if (str == QStringLiteral("餐饮") || str == "Dining") {
        return Dining;
    }
    return Other;
}
