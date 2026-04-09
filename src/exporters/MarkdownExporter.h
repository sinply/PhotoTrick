#ifndef MARKDOWNEXPORTER_H
#define MARKDOWNEXPORTER_H

#include <QObject>
#include <QFile>
#include "../models/InvoiceData.h"
#include "../models/TableData.h"

class MarkdownExporter : public QObject
{
    Q_OBJECT

public:
    explicit MarkdownExporter(QObject *parent = nullptr);

    bool exportInvoices(const QString &filePath, const QList<InvoiceData> &invoices);
    bool exportTable(const QString &filePath, const TableData &table);
    bool exportTables(const QString &filePath, const QList<TableData> &tables);

    QString lastError() const { return m_lastError; }

private:
    QString formatInvoiceTable(const QList<InvoiceData> &invoices);
    QString formatInvoiceDetail(const InvoiceData &invoice);
    QString formatTable(const TableData &table);

    QString m_lastError;
};

#endif // MARKDOWNEXPORTER_H
