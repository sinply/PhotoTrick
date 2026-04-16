#ifndef CATEGORYVIEW_H
#define CATEGORYVIEW_H

#include <QWidget>
#include <QTableWidget>
#include <QTabWidget>

#include "models/InvoiceData.h"

class CategoryView : public QWidget
{
    Q_OBJECT

public:
    explicit CategoryView(QWidget *parent = nullptr);

    void addInvoiceRow(const QStringList &row, InvoiceData::Category category);
    void clearAll();

private:
    void setupUI();
    void refreshTable(QTableWidget *table, const QList<QStringList> &data);

    QTabWidget *m_tabWidget;
    QTableWidget *m_tableAll;
    QTableWidget *m_tableTransportation;
    QTableWidget *m_tableAccommodation;
    QTableWidget *m_tableDining;

    QList<QStringList> m_dataAll;
    QList<QStringList> m_dataTransportation;
    QList<QStringList> m_dataAccommodation;
    QList<QStringList> m_dataDining;
};

#endif // CATEGORYVIEW_H
