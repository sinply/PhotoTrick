#ifndef CATEGORYVIEW_H
#define CATEGORYVIEW_H

#include <QWidget>
#include <QTableWidget>
#include <QTabWidget>

#include "models/InvoiceData.h"
#include "models/ItineraryData.h"

class CategoryView : public QWidget
{
    Q_OBJECT

public:
    explicit CategoryView(QWidget *parent = nullptr);

    void addInvoiceRow(const QStringList &row, InvoiceData::Category category);
    void addItineraryRow(const QStringList &row, ItineraryData::Type type);
    void clearAll();

private:
    void setupUI();
    void refreshTable(QTableWidget *table, const QList<QStringList> &data);

    QTabWidget *m_tabWidget;

    // Invoice tables
    QTableWidget *m_tableAll;
    QTableWidget *m_tableTransportation;
    QTableWidget *m_tableAccommodation;
    QTableWidget *m_tableDining;

    // Itinerary tables
    QTableWidget *m_tableItineraryAll;
    QTableWidget *m_tableFlight;
    QTableWidget *m_tableTrain;

    QList<QStringList> m_dataAll;
    QList<QStringList> m_dataTransportation;
    QList<QStringList> m_dataAccommodation;
    QList<QStringList> m_dataDining;

    QList<QStringList> m_dataItineraryAll;
    QList<QStringList> m_dataFlight;
    QList<QStringList> m_dataTrain;
};

#endif // CATEGORYVIEW_H
