#include "JsonExporter.h"
#include "../classifiers/InvoiceClassifier.h"
#include <QFile>
#include <QJsonArray>

JsonExporter::JsonExporter(QObject *parent)
    : QObject(parent)
{
}

bool JsonExporter::exportInvoices(const QString &filePath, const QList<InvoiceData> &invoices)
{
    QJsonArray invoiceArray;
    for (const auto &invoice : invoices) {
        invoiceArray.append(invoiceToJson(invoice));
    }

    QJsonDocument doc(invoiceArray);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        m_lastError = tr("无法打开文件: %1").arg(filePath);
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool JsonExporter::exportItineraries(const QString &filePath, const QList<ItineraryData> &itineraries)
{
    QJsonArray itineraryArray;
    for (const auto &itinerary : itineraries) {
        itineraryArray.append(itineraryToJson(itinerary));
    }

    QJsonDocument doc(itineraryArray);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        m_lastError = tr("无法打开文件: %1").arg(filePath);
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool JsonExporter::exportTable(const QString &filePath, const TableData &table)
{
    QJsonDocument doc(table.toJson());

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        m_lastError = tr("无法打开文件: %1").arg(filePath);
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool JsonExporter::exportTables(const QString &filePath, const QList<TableData> &tables)
{
    QJsonArray tableArray;
    for (const auto &table : tables) {
        tableArray.append(table.toJson());
    }

    QJsonDocument doc(tableArray);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        m_lastError = tr("无法打开文件: %1").arg(filePath);
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool JsonExporter::exportAll(const QString &filePath,
                              const QList<InvoiceData> &invoices,
                              const QList<ItineraryData> &itineraries,
                              const QList<TableData> &tables)
{
    QJsonObject root;

    // 发票数据
    QJsonArray invoiceArray;
    for (const auto &invoice : invoices) {
        invoiceArray.append(invoiceToJson(invoice));
    }
    root["invoices"] = invoiceArray;

    // 行程单数据
    QJsonArray itineraryArray;
    for (const auto &itinerary : itineraries) {
        itineraryArray.append(itineraryToJson(itinerary));
    }
    root["itineraries"] = itineraryArray;

    // 表格数据
    QJsonArray tableArray;
    for (const auto &table : tables) {
        tableArray.append(table.toJson());
    }
    root["tables"] = tableArray;

    // 元数据
    root["exportTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["version"] = "1.0";

    QJsonDocument doc(root);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        m_lastError = tr("无法打开文件: %1").arg(filePath);
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

QJsonObject JsonExporter::invoiceToJson(const InvoiceData &invoice)
{
    QJsonObject obj;
    obj["invoiceType"] = invoice.invoiceType;
    obj["invoiceNumber"] = invoice.invoiceNumber;
    obj["date"] = invoice.invoiceDate.toString("yyyy-MM-dd");
    obj["category"] = InvoiceClassifier::categoryToString(invoice.category);
    obj["totalAmount"] = invoice.totalAmount;
    obj["amountWithoutTax"] = invoice.amountWithoutTax;
    obj["taxAmount"] = invoice.taxAmount;
    obj["taxRate"] = invoice.taxRate;
    obj["buyerName"] = invoice.buyerName;
    obj["buyerTaxId"] = invoice.buyerTaxId;
    obj["sellerName"] = invoice.sellerName;
    obj["sellerTaxId"] = invoice.sellerTaxId;

    if (!invoice.departure.isEmpty()) {
        obj["departure"] = invoice.departure;
    }
    if (!invoice.destination.isEmpty()) {
        obj["destination"] = invoice.destination;
    }

    // 明细项目
    if (!invoice.items.isEmpty()) {
        QJsonArray itemsArray;
        for (const auto &item : invoice.items) {
            QJsonObject itemObj;
            itemObj["description"] = item.description;
            itemObj["quantity"] = item.quantity;
            itemObj["unitPrice"] = item.unitPrice;
            itemObj["amount"] = item.amount;
            itemsArray.append(itemObj);
        }
        obj["items"] = itemsArray;
    }

    return obj;
}

QJsonObject JsonExporter::itineraryToJson(const ItineraryData &itinerary)
{
    QJsonObject obj;
    obj["type"] = itinerary.typeToString();
    obj["passengerName"] = itinerary.passengerName;
    obj["departure"] = itinerary.departure;
    obj["destination"] = itinerary.destination;

    if (itinerary.departureTime.isValid()) {
        obj["departureTime"] = itinerary.departureTime.toString(Qt::ISODate);
    }
    if (itinerary.arrivalTime.isValid()) {
        obj["arrivalTime"] = itinerary.arrivalTime.toString(Qt::ISODate);
    }

    obj["flightTrainNo"] = itinerary.flightTrainNo;
    obj["seatClass"] = itinerary.seatClass;
    obj["seatNumber"] = itinerary.seatNumber;
    obj["price"] = itinerary.price;
    obj["taxAmount"] = itinerary.taxAmount;

    return obj;
}
