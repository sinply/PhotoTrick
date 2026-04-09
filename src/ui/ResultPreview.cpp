#include "ResultPreview.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QHeaderView>

ResultPreview::ResultPreview(QWidget *parent)
    : QWidget(parent)
    , m_tabWidget(nullptr)
    , m_tableWidget(nullptr)
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

    // Tab widget for categories
    m_tabWidget = new QTabWidget(this);

    // Create table for each category
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(8);
    m_tableWidget->setHorizontalHeaderLabels({
        tr("发票号码"), tr("类型"), tr("金额"), tr("税额"),
        tr("税率"), tr("日期"), tr("销方"), tr("备注")
    });
    m_tableWidget->horizontalHeader()->setStretchLastSection(true);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_tabWidget->addTab(m_tableWidget, tr("全部"));
    m_tabWidget->addTab(new QTableWidget(this), tr("交通"));
    m_tabWidget->addTab(new QTableWidget(this), tr("住宿"));
    m_tabWidget->addTab(new QTableWidget(this), tr("餐饮"));

    mainLayout->addWidget(m_tabWidget);

    // Summary bar
    QHBoxLayout *summaryLayout = new QHBoxLayout();
    m_labelCount = new QLabel(tr("发票数量: 0"), this);
    m_labelTotal = new QLabel(tr("合计: ¥0.00"), this);
    m_labelTaxTotal = new QLabel(tr("税额合计: ¥0.00"), this);

    summaryLayout->addWidget(m_labelCount);
    summaryLayout->addWidget(new QLabel("|", this));
    summaryLayout->addWidget(m_labelTotal);
    summaryLayout->addWidget(new QLabel("|", this));
    summaryLayout->addWidget(m_labelTaxTotal);
    summaryLayout->addStretch();

    mainLayout->addLayout(summaryLayout);
}

void ResultPreview::setupConnections()
{
}

void ResultPreview::setTableData(const QStringList &headers, const QList<QStringList> &rows)
{
    m_tableWidget->clearContents();
    m_tableWidget->setRowCount(rows.size());

    if (!headers.isEmpty()) {
        m_tableWidget->setColumnCount(headers.size());
        m_tableWidget->setHorizontalHeaderLabels(headers);
    }

    for (int row = 0; row < rows.size(); ++row) {
        const QStringList &rowData = rows[row];
        for (int col = 0; col < rowData.size() && col < m_tableWidget->columnCount(); ++col) {
            m_tableWidget->setItem(row, col, new QTableWidgetItem(rowData[col]));
        }
    }

    m_tableWidget->resizeColumnsToContents();
    m_labelCount->setText(tr("发票数量: %1").arg(rows.size()));
}

void ResultPreview::clearData()
{
    m_tableWidget->clearContents();
    m_tableWidget->setRowCount(0);
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
