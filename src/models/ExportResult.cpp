#include "ExportResult.h"
#include <QJsonArray>

void ExportResult::calculateSummary()
{
    totalAmount = 0.0;
    totalTax = 0.0;
    transportationCount = 0;
    accommodationCount = 0;
    diningCount = 0;

    for (const InvoiceData &inv : invoices) {
        totalAmount += inv.totalAmount;
        totalTax += inv.taxAmount;
        switch (inv.category) {
        case InvoiceData::Transportation: transportationCount++; break;
        case InvoiceData::Accommodation:  accommodationCount++; break;
        case InvoiceData::Dining:         diningCount++; break;
        default: break;
        }
    }

    for (const ItineraryData &it : itineraries) {
        totalAmount += it.price;
        totalTax += it.taxAmount;
        transportationCount++;
    }
}

QJsonObject ExportResult::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("totalAmount")] = totalAmount;
    obj[QStringLiteral("totalTax")] = totalTax;
    obj[QStringLiteral("transportationCount")] = transportationCount;
    obj[QStringLiteral("accommodationCount")] = accommodationCount;
    obj[QStringLiteral("diningCount")] = diningCount;
    obj[QStringLiteral("processingTimeMs")] = processingTimeMs;

    QJsonArray invArray;
    for (const InvoiceData &inv : invoices) {
        invArray.append(inv.toJson());
    }
    obj[QStringLiteral("invoices")] = invArray;

    QJsonArray itArray;
    for (const ItineraryData &it : itineraries) {
        itArray.append(it.toJson());
    }
    obj[QStringLiteral("itineraries")] = itArray;

    QJsonArray tabArray;
    for (const TableData &tab : tables) {
        tabArray.append(tab.toJson());
    }
    obj[QStringLiteral("tables")] = tabArray;

    QJsonArray errArray;
    for (const QString &err : errors) {
        errArray.append(err);
    }
    obj[QStringLiteral("errors")] = errArray;

    return obj;
}

ExportResult ExportResult::fromJson(const QJsonObject &json)
{
    ExportResult result;
    result.totalAmount = json[QStringLiteral("totalAmount")].toDouble();
    result.totalTax = json[QStringLiteral("totalTax")].toDouble();
    result.transportationCount = json[QStringLiteral("transportationCount")].toInt();
    result.accommodationCount = json[QStringLiteral("accommodationCount")].toInt();
    result.diningCount = json[QStringLiteral("diningCount")].toInt();
    result.processingTimeMs = json[QStringLiteral("processingTimeMs")].toInteger();

    const QJsonArray invArray = json[QStringLiteral("invoices")].toArray();
    for (const QJsonValue &v : invArray) {
        result.invoices.append(InvoiceData::fromJson(v.toObject()));
    }

    const QJsonArray itArray = json[QStringLiteral("itineraries")].toArray();
    for (const QJsonValue &v : itArray) {
        result.itineraries.append(ItineraryData::fromJson(v.toObject()));
    }

    const QJsonArray tabArray = json[QStringLiteral("tables")].toArray();
    for (const QJsonValue &v : tabArray) {
        result.tables.append(TableData::fromJson(v.toObject()));
    }

    const QJsonArray errArray = json[QStringLiteral("errors")].toArray();
    for (const QJsonValue &v : errArray) {
        result.errors.append(v.toString());
    }

    return result;
}
