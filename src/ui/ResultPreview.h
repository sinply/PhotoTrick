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
    enum DisplayMode {
        InvoiceMode,    // 发票识别模式 - 显示分类标签页和汇总
        TableMode,      // 表格提取模式 - 简单表格，无分类
        ItineraryMode   // 行程单模式 - 行程信息，无分类
    };

    explicit ResultPreview(QWidget *parent = nullptr);

    void setDisplayMode(DisplayMode mode);
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
    void updateUIForMode();

    DisplayMode m_displayMode;
    QTabWidget *m_tabWidget;
    QTableWidget *m_tableWidget;
    QTableWidget *m_tableTransportation;
    QTableWidget *m_tableAccommodation;
    QTableWidget *m_tableDining;

    // Summary
    QWidget *m_summaryWidget;
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
