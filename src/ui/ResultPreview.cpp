#include "ResultPreview.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QHeaderView>

ResultPreview::ResultPreview(QWidget *parent)
    : QWidget(parent)
    , m_displayMode(InvoiceMode)
    , m_tabWidget(nullptr)
    , m_tableWidget(nullptr)
    , m_summaryWidget(nullptr)
    , m_labelTotal(nullptr)
    , m_labelTaxTotal(nullptr)
    , m_labelCount(nullptr)
{
    setupUI();
    setupConnections();
}

void ResultPreview::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);

    // Header with export button
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel(tr("处理结果"), this);
    titleLabel->setStyleSheet("font-weight: bold;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    // Export dropdown
    QMenu *exportMenu = new QMenu(this);
    exportMenu->addAction(tr("Markdown (.md)"), this, &ResultPreview::onExportMarkdown);
    exportMenu->addAction(tr("CSV (.csv)"), this, &ResultPreview::onExportExcel);
    exportMenu->addAction(tr("JSON (.json)"), this, &ResultPreview::onExportJson);

    QPushButton *btnExport = new QPushButton(tr("导出"), this);
    btnExport->setMenu(exportMenu);
    headerLayout->addWidget(btnExport);

    mainLayout->addLayout(headerLayout);

    // Tab widget for categories (only for invoice mode)
    m_tabWidget = new QTabWidget(this);

    // Create main table
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->horizontalHeader()->setStretchLastSection(true);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Setup tabs - only main table (category views handled by CategoryView)
    m_tabWidget->addTab(m_tableWidget, tr("全部"));

    mainLayout->addWidget(m_tabWidget);

    // Summary bar (only for invoice mode)
    m_summaryWidget = new QWidget(this);
    QHBoxLayout *summaryLayout = new QHBoxLayout(m_summaryWidget);
    summaryLayout->setContentsMargins(0, 0, 0, 0);
    m_labelCount = new QLabel(tr("发票数量: 0"), m_summaryWidget);
    m_labelTotal = new QLabel(tr("合计: ¥0.00"), m_summaryWidget);
    m_labelTaxTotal = new QLabel(tr("税额合计: ¥0.00"), m_summaryWidget);

    summaryLayout->addWidget(m_labelCount);
    summaryLayout->addWidget(new QLabel("|", m_summaryWidget));
    summaryLayout->addWidget(m_labelTotal);
    summaryLayout->addWidget(new QLabel("|", m_summaryWidget));
    summaryLayout->addWidget(m_labelTaxTotal);
    summaryLayout->addStretch();

    mainLayout->addWidget(m_summaryWidget);
}

void ResultPreview::setupConnections()
{
}

void ResultPreview::setDisplayMode(DisplayMode mode)
{
    m_displayMode = mode;
    updateUIForMode();
}

void ResultPreview::updateUIForMode()
{
    switch (m_displayMode) {
    case InvoiceMode:
        m_tabWidget->setTabText(0, tr("全部"));
        m_summaryWidget->show();
        m_labelCount->setText(tr("发票数量: 0"));
        break;

    case TableMode:
        m_tabWidget->setTabText(0, tr("表格数据"));
        m_summaryWidget->hide();
        break;

    case ItineraryMode:
        m_tabWidget->setTabText(0, tr("行程信息"));
        m_summaryWidget->show();
        m_labelCount->setText(tr("行程单数量: 0"));
        m_labelTotal->setText(tr("合计: ¥0.00"));
        m_labelTaxTotal->setText(tr("税额合计: ¥0.00"));
        break;
    }
}

void ResultPreview::setTableData(const QStringList &headers, const QList<QStringList> &rows)
{
    m_tableWidget->clearContents();
    m_tableWidget->setRowCount(rows.size());

    if (!headers.isEmpty()) {
        m_tableWidget->setColumnCount(headers.size());
        m_tableWidget->setHorizontalHeaderLabels(headers);
    }

    double totalAmount = 0.0;
    double totalTax = 0.0;
    m_itineraryGrandTotal = 0.0;

    for (int row = 0; row < rows.size(); ++row) {
        const QStringList &rowData = rows[row];
        for (int col = 0; col < rowData.size() && col < m_tableWidget->columnCount(); ++col) {
            m_tableWidget->setItem(row, col, new QTableWidgetItem(rowData[col]));
        }

        if (m_displayMode == InvoiceMode && rowData.size() > 2) {
            bool ok = false;
            double amount = rowData[2].toDouble(&ok);
            if (ok) totalAmount += amount;
            if (rowData.size() > 3) {
                double tax = rowData[3].toDouble(&ok);
                if (ok) totalTax += tax;
            }
        } else if (m_displayMode == ItineraryMode && rowData.size() > 8) {
            bool ok = false;
            // Price at index 8, tax at index 9, total at index 13
            double price = rowData[8].toDouble(&ok);
            if (ok) totalAmount += price;
            if (rowData.size() > 9) {
                double tax = rowData[9].toDouble(&ok);
                if (ok) totalTax += tax;
            }
            // Use totalAmount (index 13) for the grand total if available
            if (rowData.size() > 13) {
                double grandTotal = rowData[13].toDouble(&ok);
                if (ok) m_itineraryGrandTotal += grandTotal;
            } else {
                m_itineraryGrandTotal += price; // fallback
            }
        }
    }

    m_tableWidget->resizeColumnsToContents();

    if (m_displayMode == InvoiceMode) {
        m_labelCount->setText(tr("发票数量: %1").arg(rows.size()));
        m_labelTotal->setText(tr("合计: ¥%1").arg(totalAmount, 0, 'f', 2));
        m_labelTaxTotal->setText(tr("税额合计: ¥%1").arg(totalTax, 0, 'f', 2));
    } else if (m_displayMode == ItineraryMode) {
        m_labelCount->setText(tr("行程单数量: %1").arg(rows.size()));
        m_labelTotal->setText(tr("合计: ¥%1").arg(m_itineraryGrandTotal > 0.0 ? m_itineraryGrandTotal : totalAmount, 0, 'f', 2));
        m_labelTaxTotal->setText(tr("票价: ¥%1").arg(totalAmount, 0, 'f', 2));
    }
}

void ResultPreview::clearData()
{
    m_tableWidget->clearContents();
    m_tableWidget->setRowCount(0);

    if (m_displayMode == ItineraryMode) {
        m_labelCount->setText(tr("行程单数量: 0"));
        m_labelTotal->setText(tr("合计: ¥0.00"));
        m_labelTaxTotal->setText(tr("票价: ¥0.00"));
    } else {
        m_labelCount->setText(tr("发票数量: 0"));
        m_labelTotal->setText(tr("合计: ¥0.00"));
        m_labelTaxTotal->setText(tr("税额合计: ¥0.00"));
    }
    m_itineraryGrandTotal = 0.0;
}

void ResultPreview::onExportMarkdown()
{
    emit exportRequested("markdown");
}

void ResultPreview::onExportExcel()
{
    emit exportRequested("excel");
}

void ResultPreview::onExportJson()
{
    emit exportRequested("json");
}
