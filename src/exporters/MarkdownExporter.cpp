#include "MarkdownExporter.h"
#include "../classifiers/InvoiceClassifier.h"
#include <QTextStream>
#include <QDate>

MarkdownExporter::MarkdownExporter(QObject *parent)
    : QObject(parent)
{
}

bool MarkdownExporter::exportInvoices(const QString &filePath, const QList<InvoiceData> &invoices)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = tr("无法打开文件: %1").arg(filePath);
        return false;
    }

    QTextStream out(&file);
    // Qt6 uses UTF-8 by default

    // 标题
    out << "# 发票汇总报告\n\n";
    out << "生成时间: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n\n";

    // 按分类统计
    double totalAmount = 0;
    double totalTax = 0;
    QMap<InvoiceData::Category, QList<InvoiceData>> categorized;

    for (const auto &invoice : invoices) {
        categorized[invoice.category].append(invoice);
        totalAmount += invoice.totalAmount;
        totalTax += invoice.taxAmount;
    }

    // 汇总表
    out << "## 汇总统计\n\n";
    out << "| 分类 | 数量 | 金额合计 | 税额合计 |\n";
    out << "|------|------|----------|----------|\n";

    auto categories = {InvoiceData::Transportation, InvoiceData::Accommodation,
                       InvoiceData::Dining, InvoiceData::Other};

    for (auto cat : categories) {
        if (categorized.contains(cat)) {
            const auto &list = categorized[cat];
            double catAmount = 0, catTax = 0;
            for (const auto &inv : list) {
                catAmount += inv.totalAmount;
                catTax += inv.taxAmount;
            }
            out << QString("| %1 | %2 | ¥%3 | ¥%4 |\n")
                   .arg(InvoiceClassifier::categoryToString(cat))
                   .arg(list.size())
                   .arg(catAmount, 0, 'f', 2)
                   .arg(catTax, 0, 'f', 2);
        }
    }

    out << QString("| **合计** | **%1** | **¥%2** | **¥%3** |\n\n")
           .arg(invoices.size())
           .arg(totalAmount, 0, 'f', 2)
           .arg(totalTax, 0, 'f', 2);

    // 明细表格
    out << "## 发票明细\n\n";
    out << formatInvoiceTable(invoices);

    // 各分类详情
    out << "\n## 分类详情\n\n";

    for (auto cat : categories) {
        if (categorized.contains(cat) && !categorized[cat].isEmpty()) {
            out << QString("### %1\n\n").arg(InvoiceClassifier::categoryToString(cat));
            out << formatInvoiceTable(categorized[cat]);
            out << "\n";
        }
    }

    file.close();
    return true;
}

QString MarkdownExporter::formatInvoiceTable(const QList<InvoiceData> &invoices)
{
    QString result;
    result += "| 发票号码 | 类型 | 金额 | 税额 | 税率 | 日期 | 销售方 |\n";
    result += "|----------|------|------|------|------|------|--------|\n";

    for (const auto &invoice : invoices) {
        result += QString("| %1 | %2 | ¥%3 | ¥%4 | %5% | %6 | %7 |\n")
                     .arg(invoice.invoiceNumber)
                     .arg(InvoiceClassifier::categoryToString(invoice.category))
                     .arg(invoice.totalAmount, 0, 'f', 2)
                     .arg(invoice.taxAmount, 0, 'f', 2)
                     .arg(invoice.taxRate, 0, 'f', 0)
                     .arg(invoice.invoiceDate.toString("yyyy-MM-dd"))
                     .arg(invoice.sellerName);
    }

    return result;
}

QString MarkdownExporter::formatInvoiceDetail(const InvoiceData &invoice)
{
    QString result;
    result += QString("### 发票 %1\n\n").arg(invoice.invoiceNumber);
    result += QString("- **发票类型**: %1\n").arg(invoice.invoiceType);
    result += QString("- **分类**: %1\n").arg(InvoiceClassifier::categoryToString(invoice.category));
    result += QString("- **开票日期**: %1\n").arg(invoice.invoiceDate.toString("yyyy-MM-dd"));
    result += QString("- **价税合计**: ¥%1\n").arg(invoice.totalAmount, 0, 'f', 2);
    result += QString("- **不含税金额**: ¥%1\n").arg(invoice.amountWithoutTax, 0, 'f', 2);
    result += QString("- **税额**: ¥%1\n").arg(invoice.taxAmount, 0, 'f', 2);
    result += QString("- **税率**: %1%\n").arg(invoice.taxRate, 0, 'f', 0);
    result += "\n";
    result += QString("- **购买方**: %1\n").arg(invoice.buyerName);
    result += QString("- **销售方**: %1\n").arg(invoice.sellerName);

    if (!invoice.items.isEmpty()) {
        result += "\n**明细项目**:\n\n";
        result += "| 项目 | 数量 | 单价 | 金额 |\n";
        result += "|------|------|------|------|\n";
        for (const auto &item : invoice.items) {
            result += QString("| %1 | %2 | ¥%3 | ¥%4 |\n")
                         .arg(item.description)
                         .arg(item.quantity)
                         .arg(item.unitPrice, 0, 'f', 2)
                         .arg(item.amount, 0, 'f', 2);
        }
    }

    return result;
}

bool MarkdownExporter::exportTable(const QString &filePath, const TableData &table)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = tr("无法打开文件: %1").arg(filePath);
        return false;
    }

    QTextStream out(&file);

    if (!table.title.isEmpty()) {
        out << "# " << table.title << "\n\n";
    }

    out << formatTable(table);

    file.close();
    return true;
}

bool MarkdownExporter::exportTables(const QString &filePath, const QList<TableData> &tables)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = tr("无法打开文件: %1").arg(filePath);
        return false;
    }

    QTextStream out(&file);

    out << "# 表格数据\n\n";

    int index = 1;
    for (const auto &table : tables) {
        if (!table.title.isEmpty()) {
            out << QString("## %1\n\n").arg(table.title);
        } else {
            out << QString("## 表格 %1\n\n").arg(index);
        }
        out << formatTable(table);
        out << "\n";
        index++;
    }

    file.close();
    return true;
}

QString MarkdownExporter::formatTable(const TableData &table)
{
    QString result;

    // 表头
    if (!table.headers.isEmpty()) {
        result += "|";
        for (const auto &header : table.headers) {
            result += QString(" %1 |").arg(header);
        }
        result += "\n|";
        for (int i = 0; i < table.headers.size(); ++i) {
            result += "------|";
        }
        result += "\n";
    }

    // 数据行
    for (const auto &row : table.rows) {
        result += "|";
        for (const auto &cell : row) {
            result += QString(" %1 |").arg(cell.text);
        }
        result += "\n";
    }

    return result;
}
