#include "JsonExporter.h"
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
    return invoice.toJson();
}

QJsonObject JsonExporter::itineraryToJson(const ItineraryData &itinerary)
{
    return itinerary.toJson();
}
