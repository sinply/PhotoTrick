#ifndef CATEGORYVIEW_H
#define CATEGORYVIEW_H

#include <QWidget>
#include <QTableWidget>

class CategoryView : public QWidget
{
    Q_OBJECT

public:
    enum Category {
        All,
        Transportation,
        Accommodation,
        Dining
    };

    explicit CategoryView(QWidget *parent = nullptr);

    void setCategoryData(Category category, const QList<QStringList> &data);
    void clearAll();

private:
    void setupUI();

    QTableWidget *m_tableAll;
    QTableWidget *m_tableTransportation;
    QTableWidget *m_tableAccommodation;
    QTableWidget *m_tableDining;
};

#endif // CATEGORYVIEW_H
