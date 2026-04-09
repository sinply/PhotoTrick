#include "CsvExporter.h"
#include "../classifiers/InvoiceClassifier.h"
#include <QFile>
#include <QTextStream>

CsvExporter::CsvExporter(QObject *parent)
    : QObject(parent)
{
}

bool CsvExporter::exportInvoices(const QString &filePath, const QList<InvoiceData> &invoices)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = tr("无法打开文件: %1").arg(filePath);
        return false;
    }

    // UTF-8 BOM for Excel compatibility
    file.write("\xEF\xBB\xBF");

    QTextStream out(&file);
    // Qt6 uses UTF-8 by default

    // 表头
    QStringList headers = {
        QStringLiteral("发票号码"),
        QStringLiteral("发票类型"),
        QStringLiteral("分类"),
        QStringLiteral("开票日期"),
        QStringLiteral("价税合计"),
        QStringLiteral("不含税金额"),
        QStringLiteral("税额"),
        QStringLiteral("税率"),
        QStringLiteral("购买方"),
        QStringLiteral("销售方"),
        QStringLiteral("出发地"),
        QStringLiteral("目的地")
    };
    out << headers.join(",") << "\n";

    // 数据行
    for (const auto &invoice : invoices) {
        QStringList row;
        row << escapeCsvField(invoice.invoiceNumber);
        row << escapeCsvField(invoice.invoiceType);
        row << escapeCsvField(InvoiceClassifier::categoryToString(invoice.category));
        row << escapeCsvField(invoice.invoiceDate.toString("yyyy-MM-dd"));
        row << QString::number(invoice.totalAmount, 'f', 2);
        row << QString::number(invoice.amountWithoutTax, 'f', 2);
        row << QString::number(invoice.taxAmount, 'f', 2);
        row << QString::number(invoice.taxRate, 'f', 2);
        row << escapeCsvField(invoice.buyerName);
        row << escapeCsvField(invoice.sellerName);
        row << escapeCsvField(invoice.departure);
        row << escapeCsvField(invoice.destination);

        out << row.join(",") << "\n";
    }

    file.close();
    return true;
}

bool CsvExporter::exportTable(const QString &filePath, const TableData &table)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = tr("无法打开文件: %1").arg(filePath);
        return false;
    }

    // UTF-8 BOM for Excel compatibility
    file.write("\xEF\xBB\xBF");

    QTextStream out(&file);
    // Qt6 uses UTF-8 by default

    // 表头
    if (!table.headers.isEmpty()) {
        QStringList escapedHeaders;
        for (const auto &header : table.headers) {
            escapedHeaders << escapeCsvField(header);
        }
        out << escapedHeaders.join(",") << "\n";
    }

    // 数据行
    for (const auto &row : table.rows) {
        QStringList escapedRow;
        for (const auto &cell : row) {
            escapedRow << escapeCsvField(cell.text);
        }
        out << escapedRow.join(",") << "\n";
    }

    file.close();
    return true;
}

QString CsvExporter::escapeCsvField(const QString &field)
{
    if (field.contains(',') || field.contains('"') || field.contains('\n')) {
        QString escaped = field;
        escaped.replace("\"", "\"\"");
        return "\"" + escaped + "\"";
    }
    return field;
}
