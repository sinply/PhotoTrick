#include "ProcessingPanel.h"
#include "ApiConfigWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDateTime>
#include <QTextCursor>

ProcessingPanel::ProcessingPanel(QWidget *parent)
    : QWidget(parent)
    , m_groupProcessing(nullptr)
    , m_radioTable(nullptr)
    , m_radioInvoice(nullptr)
    , m_radioItinerary(nullptr)
    , m_groupOcr(nullptr)
    , m_comboBackend(nullptr)
    , m_comboModel(nullptr)
    , m_btnConfigureApi(nullptr)
    , m_labelServerStatus(nullptr)
    , m_labelApiStatus(nullptr)
    , m_textLog(nullptr)
    , m_btnStart(nullptr)
    , m_btnCancel(nullptr)
{
    setupUI();
    setupConnections();
}

void ProcessingPanel::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(10);

    // Processing options group
    m_groupProcessing = new QGroupBox(tr("处理选项"), this);
    QVBoxLayout *processingLayout = new QVBoxLayout(m_groupProcessing);

    m_radioTable = new QRadioButton(tr("表格提取"), this);
    m_radioInvoice = new QRadioButton(tr("发票识别"), this);
    m_radioItinerary = new QRadioButton(tr("行程单识别"), this);

    m_radioInvoice->setChecked(true);

    processingLayout->addWidget(m_radioTable);
    processingLayout->addWidget(m_radioInvoice);
    processingLayout->addWidget(m_radioItinerary);

    mainLayout->addWidget(m_groupProcessing);

    // OCR settings group
    m_groupOcr = new QGroupBox(tr("OCR设置"), this);
    QVBoxLayout *ocrLayout = new QVBoxLayout(m_groupOcr);

    QHBoxLayout *backendLayout = new QHBoxLayout();
    backendLayout->addWidget(new QLabel(tr("后端:"), this));
    m_comboBackend = new QComboBox(this);
    m_comboBackend->addItem(tr("PaddleOCR本地"), "paddle_local");
    m_comboBackend->addItem(tr("Claude兼容格式"), "claude_format");
    m_comboBackend->addItem(tr("OpenAI兼容格式"), "openai_format");
    backendLayout->addWidget(m_comboBackend);
    ocrLayout->addLayout(backendLayout);

    QHBoxLayout *modelLayout = new QHBoxLayout();
    modelLayout->addWidget(new QLabel(tr("模型:"), this));
    m_comboModel = new QComboBox(this);
    m_comboModel->setEnabled(false);
    modelLayout->addWidget(m_comboModel);
    ocrLayout->addLayout(modelLayout);

    // Server status indicator
    QHBoxLayout *statusLayout = new QHBoxLayout();
    statusLayout->addWidget(new QLabel(tr("服务器状态:"), this));
    m_labelServerStatus = new QLabel(tr("未启动"), this);
    m_labelServerStatus->setStyleSheet("QLabel { color: gray; font-weight: bold; }");
    statusLayout->addWidget(m_labelServerStatus);
    statusLayout->addStretch();
    ocrLayout->addLayout(statusLayout);

    m_btnConfigureApi = new QPushButton(tr("配置API..."), this);
    m_btnConfigureApi->setProperty("secondary", true);
    ocrLayout->addWidget(m_btnConfigureApi);

    // API status indicator
    QHBoxLayout *apiStatusLayout = new QHBoxLayout();
    apiStatusLayout->addWidget(new QLabel(tr("API状态:"), this));
    m_labelApiStatus = new QLabel(tr("未配置"), this);
    m_labelApiStatus->setStyleSheet("QLabel { color: gray; font-weight: bold; }");
    apiStatusLayout->addWidget(m_labelApiStatus);
    apiStatusLayout->addStretch();
    ocrLayout->addLayout(apiStatusLayout);

    mainLayout->addWidget(m_groupOcr);

    // Processing log
    QGroupBox *groupLog = new QGroupBox(tr("处理日志"), this);
    QVBoxLayout *logLayout = new QVBoxLayout(groupLog);

    m_textLog = new QTextEdit(this);
    m_textLog->setReadOnly(true);
    m_textLog->setMaximumHeight(120);
    m_textLog->setStyleSheet("QTextEdit { background-color: #f5f5f5; color: #333333; font-family: Consolas, 'Courier New', monospace; font-size: 11px; border: 1px solid #ddd; border-radius: 4px; padding: 4px; }");
    logLayout->addWidget(m_textLog);

    mainLayout->addWidget(groupLog);

    mainLayout->addStretch();

    // Action buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_btnStart = new QPushButton(tr("开始处理"), this);
    m_btnCancel = new QPushButton(tr("取消"), this);
    m_btnCancel->setProperty("secondary", true);
    m_btnCancel->setEnabled(false);

    buttonLayout->addWidget(m_btnStart);
    buttonLayout->addWidget(m_btnCancel);

    mainLayout->addLayout(buttonLayout);
}

void ProcessingPanel::setupConnections()
{
    connect(m_btnStart, &QPushButton::clicked, this, &ProcessingPanel::startProcessing);
    connect(m_btnCancel, &QPushButton::clicked, this, &ProcessingPanel::cancelProcessing);
    connect(m_btnConfigureApi, &QPushButton::clicked, this, &ProcessingPanel::onConfigureApi);
    connect(m_comboBackend, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ProcessingPanel::onBackendChanged);
}

ProcessingPanel::ProcessingMode ProcessingPanel::processingMode() const
{
    if (m_radioTable->isChecked()) return TableExtraction;
    if (m_radioInvoice->isChecked()) return InvoiceRecognition;
    return ItineraryRecognition;
}

QString ProcessingPanel::ocrBackend() const
{
    return m_comboBackend->currentData().toString();
}

QString ProcessingPanel::apiKey() const
{
    return m_apiKey;
}

QString ProcessingPanel::baseUrl() const
{
    return m_baseUrl;
}

QString ProcessingPanel::model() const
{
    return m_comboModel->currentText();
}

void ProcessingPanel::setApiKey(const QString &key)
{
    m_apiKey = key;
}

void ProcessingPanel::setBaseUrl(const QString &url)
{
    m_baseUrl = url;
}

void ProcessingPanel::setModel(const QString &model)
{
    int index = m_comboModel->findText(model);
    if (index >= 0) {
        m_comboModel->setCurrentIndex(index);
    } else {
        m_comboModel->setEditText(model);
    }
}

void ProcessingPanel::onBackendChanged(int index)
{
    QString backend = m_comboBackend->itemData(index).toString();

    m_comboModel->clear();
    m_comboModel->setEnabled(backend != "paddle_local");

    if (backend == "claude_format" || backend == "openai_format") {
        m_comboModel->addItem("deepseek-chat");
        m_comboModel->addItem("glm-4v");
    }

    // Emit signal for MainWindow to handle OCR server auto-start
    emit backendChanged(backend);
}

void ProcessingPanel::onConfigureApi()
{
    ApiConfigWidget dialog(this);
    dialog.setApiKey(m_apiKey);
    dialog.setBaseUrl(m_baseUrl);
    dialog.setModel(m_comboModel->currentText());

    if (dialog.exec() == QDialog::Accepted) {
        m_apiKey = dialog.apiKey();
        m_baseUrl = dialog.baseUrl();

        // 更新模型
        if (!dialog.model().isEmpty()) {
            int index = m_comboModel->findText(dialog.model());
            if (index >= 0) {
                m_comboModel->setCurrentIndex(index);
            } else {
                m_comboModel->setEditText(dialog.model());
            }
        }

        // 更新API状态
        if (!m_apiKey.isEmpty()) {
            setApiStatus(ApiStatus::Configured);
        } else {
            setApiStatus(ApiStatus::NotConfigured);
        }

        emit apiSettingsChanged();
    }
}

void ProcessingPanel::setServerStatus(ServerStatus status)
{
    m_serverStatus = status;
    updateServerStatusLabel();
}

void ProcessingPanel::updateServerStatusLabel()
{
    if (!m_labelServerStatus) return;

    switch (m_serverStatus) {
    case ServerStatus::NotRunning:
        m_labelServerStatus->setText(tr("未启动"));
        m_labelServerStatus->setStyleSheet("QLabel { color: gray; font-weight: bold; }");
        break;
    case ServerStatus::Starting:
        m_labelServerStatus->setText(tr("启动中..."));
        m_labelServerStatus->setStyleSheet("QLabel { color: orange; font-weight: bold; }");
        break;
    case ServerStatus::Running:
        m_labelServerStatus->setText(tr("运行中"));
        m_labelServerStatus->setStyleSheet("QLabel { color: green; font-weight: bold; }");
        break;
    case ServerStatus::Error:
        m_labelServerStatus->setText(tr("错误"));
        m_labelServerStatus->setStyleSheet("QLabel { color: red; font-weight: bold; }");
        break;
    }
}

void ProcessingPanel::setApiStatus(ApiStatus status, const QString &message)
{
    m_apiStatus = status;
    if (!m_labelApiStatus) return;

    QString text;
    QString style;

    switch (status) {
    case ApiStatus::NotConfigured:
        text = tr("未配置");
        style = "QLabel { color: gray; font-weight: bold; }";
        break;
    case ApiStatus::Configured:
        text = tr("已配置");
        style = "QLabel { color: #0066cc; font-weight: bold; }";
        break;
    case ApiStatus::Valid:
        text = message.isEmpty() ? tr("有效 ✓") : message;
        style = "QLabel { color: green; font-weight: bold; }";
        break;
    case ApiStatus::Invalid:
        text = message.isEmpty() ? tr("无效 ✗") : message;
        style = "QLabel { color: red; font-weight: bold; }";
        break;
    }

    m_labelApiStatus->setText(text);
    m_labelApiStatus->setStyleSheet(style);
}

void ProcessingPanel::appendLog(const QString &message)
{
    if (!m_textLog) return;

    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_textLog->append(QString("[%1] %2").arg(timestamp, message));
    // 滚动到底部
    QTextCursor cursor = m_textLog->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_textLog->setTextCursor(cursor);
}

void ProcessingPanel::clearLog()
{
    if (m_textLog) {
        m_textLog->clear();
    }
}
