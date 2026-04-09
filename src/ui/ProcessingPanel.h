#ifndef PROCESSINGPANEL_H
#define PROCESSINGPANEL_H

#include <QWidget>
#include <QGroupBox>
#include <QRadioButton>
#include <QComboBox>
#include <QPushButton>

class ProcessingPanel : public QWidget
{
    Q_OBJECT

public:
    enum ProcessingMode {
        TableExtraction,
        InvoiceRecognition,
        ItineraryRecognition
    };

    explicit ProcessingPanel(QWidget *parent = nullptr);

    ProcessingMode processingMode() const;
    QString ocrBackend() const;
    QString apiKey() const;
    QString baseUrl() const;
    QString model() const;

signals:
    void startProcessing();
    void cancelProcessing();
    void settingsRequested();

private slots:
    void onBackendChanged(int index);
    void onConfigureApi();

private:
    void setupUI();
    void setupConnections();

    // Processing options
    QGroupBox *m_groupProcessing;
    QRadioButton *m_radioTable;
    QRadioButton *m_radioInvoice;
    QRadioButton *m_radioItinerary;

    // OCR settings
    QGroupBox *m_groupOcr;
    QComboBox *m_comboBackend;
    QComboBox *m_comboModel;
    QPushButton *m_btnConfigureApi;

    // Action buttons
    QPushButton *m_btnStart;
    QPushButton *m_btnCancel;

    // Current settings
    QString m_apiKey;
    QString m_baseUrl;
};

#endif // PROCESSINGPANEL_H
