#ifndef RESULTPREVIEW_H
#define RESULTPREVIEW_H

#include <QWidget>
#include <QTableWidget>
#include <QTabWidget>
#include <QLabel>
#include <QPushButton>

class ResultPreview : public QWidget
{
    Q_OBJECT

public:
    explicit ResultPreview(QWidget *parent = nullptr);

    void setTableData(const QStringList &headers, const QList<QStringList> &rows);
    void clearData();

signals:
    void exportRequested(const QString &format);

private slots:
    void onExportMarkdown();
    void onExportWord();
    void onExportExcel();
    void onExportJson();

private:
    void setupUI();
    void setupConnections();

    QTabWidget *m_tabWidget;
    QTableWidget *m_tableWidget;

    // Summary
    QLabel *m_labelTotal;
    QLabel *m_labelTaxTotal;
    QLabel *m_labelCount;

    // Export buttons
    QPushButton *m_btnExportMarkdown;
    QPushButton *m_btnExportWord;
    QPushButton *m_btnExportExcel;
    QPushButton *m_btnExportJson;
};

#endif // RESULTPREVIEW_H
