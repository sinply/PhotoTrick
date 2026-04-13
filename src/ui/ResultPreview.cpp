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
    , m_tableTransportation(nullptr)
    , m_tableAccommodation(nullptr)
    , m_tableDining(nullptr)
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
    exportMenu->addAction(tr("Word (.docx)"), this, &ResultPreview::onExportWord);
    exportMenu->addAction(tr("Excel (.csv)"), this, &ResultPreview::onExportExcel);
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

    // Create category tables (for invoice mode)
    auto createCategoryTable = [this]() -> QTableWidget* {
        QTableWidget *table = new QTableWidget(this);
        table->setColumnCount(8);
        table->setHorizontalHeaderLabels({
            tr("发票号码"), tr("类型"), tr("金额"), tr("税额"),
            tr("税率"), tr("日期"), tr("销方"), tr("备注")
        });
        table->horizontalHeader()->setStretchLastSection(true);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        return table;
    };

    m_tableTransportation = createCategoryTable();
    m_tableAccommodation = createCategoryTable();
    m_tableDining = createCategoryTable();

    // Setup tabs for invoice mode
    m_tabWidget->addTab(m_tableWidget, tr("全部"));
    m_tabWidget->addTab(m_tableTransportation, tr("交通"));
    m_tabWidget->addTab(m_tableAccommodation, tr("住宿"));
    m_tabWidget->addTab(m_tableDining, tr("餐饮"));

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
    // Show/hide category tabs based on mode
    while (m_tabWidget->count() > 1) {
        m_tabWidget->removeTab(1);
    }

    switch (m_displayMode) {
    case InvoiceMode:
        // Add category tabs
        m_tabWidget->addTab(m_tableTransportation, tr("交通"));
        m_tabWidget->addTab(m_tableAccommodation, tr("住宿"));
        m_tabWidget->addTab(m_tableDining, tr("餐饮"));
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
        m_summaryWidget->hide();
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

    for (int row = 0; row < rows.size(); ++row) {
        const QStringList &rowData = rows[row];
        for (int col = 0; col < rowData.size() && col < m_tableWidget->columnCount(); ++col) {
            m_tableWidget->setItem(row, col, new QTableWidgetItem(rowData[col]));
        }

        // Calculate totals for invoice mode (金额 column is typically at index 2)
        if (m_displayMode == InvoiceMode && rowData.size() > 2) {
            bool ok = false;
            double amount = rowData[2].toDouble(&ok);
            if (ok) {
                totalAmount += amount;
            }
            // Tax is at index 3
            if (rowData.size() > 3) {
                double tax = rowData[3].toDouble(&ok);
                if (ok) {
                    totalTax += tax;
                }
            }
        }
    }

    m_tableWidget->resizeColumnsToContents();

    if (m_displayMode == InvoiceMode) {
        m_labelCount->setText(tr("发票数量: %1").arg(rows.size()));
        m_labelTotal->setText(tr("合计: ¥%1").arg(totalAmount, 0, 'f', 2));
        m_labelTaxTotal->setText(tr("税额合计: ¥%1").arg(totalTax, 0, 'f', 2));
    }
}

void ResultPreview::clearData()
{
    m_tableWidget->clearContents();
    m_tableWidget->setRowCount(0);
    m_tableTransportation->clearContents();
    m_tableTransportation->setRowCount(0);
    m_tableAccommodation->clearContents();
    m_tableAccommodation->setRowCount(0);
    m_tableDining->clearContents();
    m_tableDining->setRowCount(0);

    m_labelCount->setText(tr("发票数量: 0"));
    m_labelTotal->setText(tr("合计: ¥0.00"));
    m_labelTaxTotal->setText(tr("税额合计: ¥0.00"));
}

void ResultPreview::onExportMarkdown()
{
    emit exportRequested("markdown");
}

void ResultPreview::onExportWord()
{
    emit exportRequested("word");
}

void ResultPreview::onExportExcel()
{
    emit exportRequested("excel");
}

void ResultPreview::onExportJson()
{
    emit exportRequested("json");
}
