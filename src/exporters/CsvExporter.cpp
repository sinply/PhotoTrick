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

    out.flush();
    file.close();
    return true;
}

bool CsvExporter::exportItineraries(const QString &filePath, const QList<ItineraryData> &itineraries)
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
        QStringLiteral("类型"),
        QStringLiteral("航班/车次"),
        QStringLiteral("乘客"),
        QStringLiteral("出发地"),
        QStringLiteral("目的地"),
        QStringLiteral("出发时间"),
        QStringLiteral("到达时间"),
        QStringLiteral("票价"),
        QStringLiteral("税额"),
        QStringLiteral("燃油附加费"),
        QStringLiteral("机建费"),
        QStringLiteral("保险费"),
        QStringLiteral("合计")
    };
    out << headers.join(",") << "\n";

    // 数据行
    for (const auto &iti : itineraries) {
        QStringList row;
        row << escapeCsvField(iti.typeToString());
        row << escapeCsvField(iti.flightTrainNo);
        row << escapeCsvField(iti.passengerName);
        row << escapeCsvField(iti.departure);
        row << escapeCsvField(iti.destination);
        row << escapeCsvField(iti.departureTime.isValid() ? iti.departureTime.toString("yyyy-MM-dd HH:mm") : QString());
        row << escapeCsvField(iti.arrivalTime.isValid() ? iti.arrivalTime.toString("yyyy-MM-dd HH:mm") : QString());
        row << QString::number(iti.price, 'f', 2);
        row << QString::number(iti.taxAmount, 'f', 2);
        row << QString::number(iti.fuelSurcharge, 'f', 2);
        row << QString::number(iti.airportTax, 'f', 2);
        row << QString::number(iti.insurance, 'f', 2);
        row << QString::number(iti.totalAmount, 'f', 2);

        out << row.join(",") << "\n";
    }

    out.flush();
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

    out.flush();
    file.close();
    return true;
}

bool CsvExporter::exportTables(const QString &filePath, const QList<TableData> &tables)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = tr("无法打开文件: %1").arg(filePath);
        return false;
    }

    // UTF-8 BOM for Excel compatibility
    file.write("\xEF\xBB\xBF");

    QTextStream out(&file);

    for (int t = 0; t < tables.size(); ++t) {
        const auto &table = tables[t];

        if (tables.size() > 1) {
            if (!table.title.isEmpty()) {
                out << escapeCsvField(table.title) << "\n";
            } else {
                out << escapeCsvField(tr("表格 %1").arg(t + 1)) << "\n";
            }
        }

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

        if (t < tables.size() - 1) {
            out << "\n";
        }
    }

    out.flush();
    file.close();
    return true;
}

bool CsvExporter::exportAll(const QString &filePath,
                            const QList<InvoiceData> &invoices,
                            const QList<ItineraryData> &itineraries,
                            const QList<TableData> &tables)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = tr("无法打开文件: %1").arg(filePath);
        return false;
    }

    // UTF-8 BOM for Excel compatibility
    file.write("\xEF\xBB\xBF");

    QTextStream out(&file);

    bool needSeparator = false;

    if (!invoices.isEmpty()) {
        if (needSeparator) out << "\n";
        // Section header
        out << escapeCsvField(tr("发票数据")) << "\n";

        // Headers
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
        needSeparator = true;
    }

    if (!itineraries.isEmpty()) {
        if (needSeparator) out << "\n";
        out << escapeCsvField(tr("行程单数据")) << "\n";

        QStringList headers = {
            QStringLiteral("类型"),
            QStringLiteral("航班/车次"),
            QStringLiteral("乘客"),
            QStringLiteral("出发地"),
            QStringLiteral("目的地"),
            QStringLiteral("出发时间"),
            QStringLiteral("到达时间"),
            QStringLiteral("票价"),
            QStringLiteral("税额"),
            QStringLiteral("燃油附加费"),
            QStringLiteral("机建费"),
            QStringLiteral("保险费"),
            QStringLiteral("合计")
        };
        out << headers.join(",") << "\n";

        for (const auto &iti : itineraries) {
            QStringList row;
            row << escapeCsvField(iti.typeToString());
            row << escapeCsvField(iti.flightTrainNo);
            row << escapeCsvField(iti.passengerName);
            row << escapeCsvField(iti.departure);
            row << escapeCsvField(iti.destination);
            row << escapeCsvField(iti.departureTime.isValid() ? iti.departureTime.toString("yyyy-MM-dd HH:mm") : QString());
            row << escapeCsvField(iti.arrivalTime.isValid() ? iti.arrivalTime.toString("yyyy-MM-dd HH:mm") : QString());
            row << QString::number(iti.price, 'f', 2);
            row << QString::number(iti.taxAmount, 'f', 2);
            row << QString::number(iti.fuelSurcharge, 'f', 2);
            row << QString::number(iti.airportTax, 'f', 2);
            row << QString::number(iti.insurance, 'f', 2);
            row << QString::number(iti.totalAmount, 'f', 2);
            out << row.join(",") << "\n";
        }
        needSeparator = true;
    }

    if (!tables.isEmpty()) {
        for (int t = 0; t < tables.size(); ++t) {
            const auto &table = tables[t];
            if (needSeparator) out << "\n";

            if (!table.title.isEmpty()) {
                out << escapeCsvField(table.title) << "\n";
            } else {
                out << escapeCsvField(tr("表格 %1").arg(t + 1)) << "\n";
            }

            if (!table.headers.isEmpty()) {
                QStringList escapedHeaders;
                for (const auto &header : table.headers) {
                    escapedHeaders << escapeCsvField(header);
                }
                out << escapedHeaders.join(",") << "\n";
            }

            for (const auto &row : table.rows) {
                QStringList escapedRow;
                for (const auto &cell : row) {
                    escapedRow << escapeCsvField(cell.text);
                }
                out << escapedRow.join(",") << "\n";
            }
            needSeparator = true;
        }
    }

    out.flush();
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
