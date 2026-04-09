#ifndef JSONEXPORTER_H
#define JSONEXPORTER_H

#include <QObject>
#include <QJsonDocument>
#include "../models/InvoiceData.h"
#include "../models/ItineraryData.h"
#include "../models/TableData.h"

class JsonExporter : public QObject
{
    Q_OBJECT

public:
    explicit JsonExporter(QObject *parent = nullptr);

    bool exportInvoices(const QString &filePath, const QList<InvoiceData> &invoices);
    bool exportItineraries(const QString &filePath, const QList<ItineraryData> &itineraries);
    bool exportTable(const QString &filePath, const TableData &table);
    bool exportTables(const QString &filePath, const QList<TableData> &tables);
    bool exportAll(const QString &filePath,
                   const QList<InvoiceData> &invoices,
                   const QList<ItineraryData> &itineraries,
                   const QList<TableData> &tables);

    QString lastError() const { return m_lastError; }

private:
    QJsonObject invoiceToJson(const InvoiceData &invoice);
    QJsonObject itineraryToJson(const ItineraryData &itinerary);

    QString m_lastError;
};

#endif // JSONEXPORTER_H
