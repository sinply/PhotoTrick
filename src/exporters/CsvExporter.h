#ifndef CSVEXPORTER_H
#define CSVEXPORTER_H

#include <QObject>
#include "../models/InvoiceData.h"
#include "../models/ItineraryData.h"
#include "../models/TableData.h"

class CsvExporter : public QObject
{
    Q_OBJECT

public:
    explicit CsvExporter(QObject *parent = nullptr);

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
    QString escapeCsvField(const QString &field);

    QString m_lastError;
};

#endif // CSVEXPORTER_H
