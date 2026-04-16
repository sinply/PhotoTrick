#include "CategoryView.h"
#include <QVBoxLayout>
#include <QHeaderView>

CategoryView::CategoryView(QWidget *parent)
    : QWidget(parent)
    , m_tabWidget(nullptr)
    , m_tableAll(nullptr)
    , m_tableTransportation(nullptr)
    , m_tableAccommodation(nullptr)
    , m_tableDining(nullptr)
{
    setupUI();
}

void CategoryView::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_tabWidget = new QTabWidget(this);

    auto createTable = [this]() -> QTableWidget* {
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

    m_tableAll = createTable();
    m_tableTransportation = createTable();
    m_tableAccommodation = createTable();
    m_tableDining = createTable();

    m_tabWidget->addTab(m_tableAll, tr("全部"));
    m_tabWidget->addTab(m_tableTransportation, tr("交通"));
    m_tabWidget->addTab(m_tableAccommodation, tr("住宿"));
    m_tabWidget->addTab(m_tableDining, tr("餐饮"));

    layout->addWidget(m_tabWidget);
}

void CategoryView::refreshTable(QTableWidget *table, const QList<QStringList> &data)
{
    table->clearContents();
    table->setRowCount(data.size());

    for (int row = 0; row < data.size(); ++row) {
        const QStringList &rowData = data[row];
        for (int col = 0; col < rowData.size() && col < table->columnCount(); ++col) {
            table->setItem(row, col, new QTableWidgetItem(rowData[col]));
        }
    }
}

void CategoryView::addInvoiceRow(const QStringList &row, InvoiceData::Category category)
{
    // Always add to "All" tab
    m_dataAll.append(row);
    refreshTable(m_tableAll, m_dataAll);

    // Add to specific category tab
    switch (category) {
    case InvoiceData::Transportation:
        m_dataTransportation.append(row);
        refreshTable(m_tableTransportation, m_dataTransportation);
        break;
    case InvoiceData::Accommodation:
        m_dataAccommodation.append(row);
        refreshTable(m_tableAccommodation, m_dataAccommodation);
        break;
    case InvoiceData::Dining:
        m_dataDining.append(row);
        refreshTable(m_tableDining, m_dataDining);
        break;
    default:
        break;
    }
}

void CategoryView::clearAll()
{
    m_dataAll.clear();
    m_dataTransportation.clear();
    m_dataAccommodation.clear();
    m_dataDining.clear();

    m_tableAll->clearContents();
    m_tableAll->setRowCount(0);
    m_tableTransportation->clearContents();
    m_tableTransportation->setRowCount(0);
    m_tableAccommodation->clearContents();
    m_tableAccommodation->setRowCount(0);
    m_tableDining->clearContents();
    m_tableDining->setRowCount(0);
}
