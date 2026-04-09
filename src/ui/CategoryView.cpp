#include "CategoryView.h"
#include <QVBoxLayout>
#include <QHeaderView>

CategoryView::CategoryView(QWidget *parent)
    : QWidget(parent)
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

    // Create tables
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

    layout->addWidget(m_tableAll);
}

void CategoryView::setCategoryData(Category category, const QList<QStringList> &data)
{
    QTableWidget *targetTable = nullptr;

    switch (category) {
    case All:
        targetTable = m_tableAll;
        break;
    case Transportation:
        targetTable = m_tableTransportation;
        break;
    case Accommodation:
        targetTable = m_tableAccommodation;
        break;
    case Dining:
        targetTable = m_tableDining;
        break;
    }

    if (!targetTable) return;

    targetTable->clearContents();
    targetTable->setRowCount(data.size());

    for (int row = 0; row < data.size(); ++row) {
        const QStringList &rowData = data[row];
        for (int col = 0; col < rowData.size() && col < targetTable->columnCount(); ++col) {
            targetTable->setItem(row, col, new QTableWidgetItem(rowData[col]));
        }
    }
}

void CategoryView::clearAll()
{
    m_tableAll->clearContents();
    m_tableAll->setRowCount(0);
    m_tableTransportation->clearContents();
    m_tableTransportation->setRowCount(0);
    m_tableAccommodation->clearContents();
    m_tableAccommodation->setRowCount(0);
    m_tableDining->clearContents();
    m_tableDining->setRowCount(0);
}
