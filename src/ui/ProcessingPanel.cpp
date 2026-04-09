#include "ProcessingPanel.h"
#include "ApiConfigWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

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

    m_btnConfigureApi = new QPushButton(tr("配置API..."), this);
    m_btnConfigureApi->setProperty("secondary", true);
    ocrLayout->addWidget(m_btnConfigureApi);

    mainLayout->addWidget(m_groupOcr);

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

void ProcessingPanel::onBackendChanged(int index)
{
    QString backend = m_comboBackend->itemData(index).toString();

    m_comboModel->clear();
    m_comboModel->setEnabled(backend != "paddle_local");

    if (backend == "claude_format") {
        m_comboModel->addItem("deepseek-chat");
        m_comboModel->addItem("glm-4");
        m_comboModel->addItem("qwen-vl-plus");
    } else if (backend == "openai_format") {
        m_comboModel->addItem("deepseek-chat");
        m_comboModel->addItem("glm-4");
        m_comboModel->addItem("qwen-vl-plus");
        m_comboModel->addItem("moonshot-v1-8k-vision");
    }
}

void ProcessingPanel::onConfigureApi()
{
    ApiConfigWidget dialog(this);
    dialog.setApiKey(m_apiKey);
    dialog.setBaseUrl(m_baseUrl);

    if (dialog.exec() == QDialog::Accepted) {
        m_apiKey = dialog.apiKey();
        m_baseUrl = dialog.baseUrl();
    }
}
