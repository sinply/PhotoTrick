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
    , m_tableItineraryAll(nullptr)
    , m_tableFlight(nullptr)
    , m_tableTrain(nullptr)
{
    setupUI();
}

void CategoryView::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_tabWidget = new QTabWidget(this);

    auto createInvoiceTable = [this]() -> QTableWidget* {
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

    auto createItineraryTable = [this]() -> QTableWidget* {
        QTableWidget *table = new QTableWidget(this);
        table->setColumnCount(8);
        table->setHorizontalHeaderLabels({
            tr("航班/车次"), tr("乘客"), tr("出发地"),
            tr("目的地"), tr("出发时间"), tr("票价"), tr("税额"), tr("合计")
        });
        table->horizontalHeader()->setStretchLastSection(true);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        return table;
    };

    // Invoice tabs
    m_tableAll = createInvoiceTable();
    m_tableTransportation = createInvoiceTable();
    m_tableAccommodation = createInvoiceTable();
    m_tableDining = createInvoiceTable();

    // Itinerary tabs
    m_tableItineraryAll = createItineraryTable();
    m_tableFlight = createItineraryTable();
    m_tableTrain = createItineraryTable();

    m_tabWidget->addTab(m_tableAll, tr("发票-全部"));
    m_tabWidget->addTab(m_tableTransportation, tr("交通"));
    m_tabWidget->addTab(m_tableAccommodation, tr("住宿"));
    m_tabWidget->addTab(m_tableDining, tr("餐饮"));
    m_tabWidget->addTab(m_tableItineraryAll, tr("行程-全部"));
    m_tabWidget->addTab(m_tableFlight, tr("机票"));
    m_tabWidget->addTab(m_tableTrain, tr("火车"));

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

void CategoryView::addItineraryRow(const QStringList &row, ItineraryData::Type type)
{
    m_dataItineraryAll.append(row);
    refreshTable(m_tableItineraryAll, m_dataItineraryAll);

    switch (type) {
    case ItineraryData::Flight:
        m_dataFlight.append(row);
        refreshTable(m_tableFlight, m_dataFlight);
        break;
    case ItineraryData::Train:
    case ItineraryData::Bus:
        m_dataTrain.append(row);
        refreshTable(m_tableTrain, m_dataTrain);
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
    m_dataItineraryAll.clear();
    m_dataFlight.clear();
    m_dataTrain.clear();

    m_tableAll->clearContents();
    m_tableAll->setRowCount(0);
    m_tableTransportation->clearContents();
    m_tableTransportation->setRowCount(0);
    m_tableAccommodation->clearContents();
    m_tableAccommodation->setRowCount(0);
    m_tableDining->clearContents();
    m_tableDining->setRowCount(0);
    m_tableItineraryAll->clearContents();
    m_tableItineraryAll->setRowCount(0);
    m_tableFlight->clearContents();
    m_tableFlight->setRowCount(0);
    m_tableTrain->clearContents();
    m_tableTrain->setRowCount(0);
}
