#ifndef PROCESSINGPANEL_H
#define PROCESSINGPANEL_H

#include <QWidget>
#include <QGroupBox>
#include <QRadioButton>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>

class ProcessingPanel : public QWidget
{
    Q_OBJECT

public:
    enum ProcessingMode {
        TableExtraction,
        InvoiceRecognition,
        ItineraryRecognition
    };

    enum class ServerStatus {
        NotRunning,
        Starting,
        Running,
        Error
    };

    explicit ProcessingPanel(QWidget *parent = nullptr);

    ProcessingMode processingMode() const;
    QString ocrBackend() const;
    QString apiKey() const;
    QString baseUrl() const;
    QString model() const;

    void setApiKey(const QString &key);
    void setBaseUrl(const QString &url);
    void setModel(const QString &model);

    void setServerStatus(ServerStatus status);

    // API状态
    enum class ApiStatus {
        NotConfigured,
        Configured,
        Valid,
        Invalid
    };
    void setApiStatus(ApiStatus status, const QString &message = QString());

    // 处理日志
    void appendLog(const QString &message);
    void clearLog();

signals:
    void startProcessing();
    void cancelProcessing();
    void settingsRequested();
    void backendChanged(const QString &backend);
    void apiSettingsChanged();  // API设置变更信号

private slots:
    void onBackendChanged(int index);
    void onConfigureApi();

private:
    void setupUI();
    void setupConnections();
    void updateServerStatusLabel();

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

    // Server status
    QLabel *m_labelServerStatus;
    ServerStatus m_serverStatus = ServerStatus::NotRunning;

    // API status
    QLabel *m_labelApiStatus;
    ApiStatus m_apiStatus = ApiStatus::NotConfigured;

    // Processing log
    QTextEdit *m_textLog;

    // Action buttons
    QPushButton *m_btnStart;
    QPushButton *m_btnCancel;

    // Current settings
    QString m_apiKey;
    QString m_baseUrl;
};

#endif // PROCESSINGPANEL_H
