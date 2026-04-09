#include "ApiConfigWidget.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>

// Provider configurations
struct ProviderConfig {
    QString name;
    QString baseUrl;
    QStringList models;
};

static const QMap<QString, ProviderConfig> PROVIDERS = {
    {"deepseek", {"DeepSeek", "https://api.deepseek.com", {"deepseek-chat", "deepseek-vision"}}},
    {"glm", {"GLM (智谱)", "https://open.bigmodel.cn/api/paas/v4", {"glm-4", "glm-4v"}}},
    {"qwen", {"通义千问", "https://dashscope.aliyuncs.com/compatible-mode", {"qwen-vl-plus", "qwen-vl-max"}}},
    {"moonshot", {"月之暗面", "https://api.moonshot.cn", {"moonshot-v1-8k-vision"}}},
    {"baichuan", {"百川", "https://api.baichuan-ai.com", {"Baichuan4"}}},
    {"custom", {"自定义", "", {}}}
};

ApiConfigWidget::ApiConfigWidget(QWidget *parent)
    : QDialog(parent)
    , m_comboFormat(nullptr)
    , m_comboProvider(nullptr)
    , m_editApiKey(nullptr)
    , m_editBaseUrl(nullptr)
    , m_comboModel(nullptr)
    , m_btnTest(nullptr)
{
    setupUI();
    setupConnections();
}

void ApiConfigWidget::setupUI()
{
    setWindowTitle(tr("API配置"));
    setMinimumSize(400, 300);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QFormLayout *form = new QFormLayout();

    // API Format
    m_comboFormat = new QComboBox(this);
    m_comboFormat->addItem(tr("Claude兼容（推荐）"), "claude");
    m_comboFormat->addItem(tr("OpenAI兼容"), "openai");
    form->addRow(tr("API格式:"), m_comboFormat);

    // Provider
    m_comboProvider = new QComboBox(this);
    for (auto it = PROVIDERS.begin(); it != PROVIDERS.end(); ++it) {
        m_comboProvider->addItem(it.value().name, it.key());
    }
    form->addRow(tr("服务商:"), m_comboProvider);

    // API Key
    m_editApiKey = new QLineEdit(this);
    m_editApiKey->setEchoMode(QLineEdit::Password);
    m_editApiKey->setPlaceholderText(tr("输入API Key"));
    form->addRow(tr("API Key:"), m_editApiKey);

    // Base URL
    m_editBaseUrl = new QLineEdit(this);
    m_editBaseUrl->setPlaceholderText(tr("自动填充，可修改"));
    form->addRow(tr("Base URL:"), m_editBaseUrl);

    // Model
    m_comboModel = new QComboBox(this);
    form->addRow(tr("模型:"), m_comboModel);

    mainLayout->addLayout(form);

    // Test button
    m_btnTest = new QPushButton(tr("测试连接"), this);
    m_btnTest->setProperty("secondary", true);
    mainLayout->addWidget(m_btnTest);

    mainLayout->addStretch();

    // Dialog buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    QPushButton *btnSave = new QPushButton(tr("保存"), this);
    QPushButton *btnCancel = new QPushButton(tr("取消"), this);
    btnCancel->setProperty("secondary", true);

    buttonLayout->addWidget(btnSave);
    buttonLayout->addWidget(btnCancel);
    mainLayout->addLayout(buttonLayout);

    connect(btnSave, &QPushButton::clicked, this, &QDialog::accept);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    // Initialize with first provider
    onProviderChanged(0);
}

void ApiConfigWidget::setupConnections()
{
    connect(m_comboFormat, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ApiConfigWidget::onFormatChanged);
    connect(m_comboProvider, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ApiConfigWidget::onProviderChanged);
    connect(m_btnTest, &QPushButton::clicked, this, &ApiConfigWidget::onTestConnection);
}

void ApiConfigWidget::setApiKey(const QString &key)
{
    m_editApiKey->setText(key);
}

void ApiConfigWidget::setBaseUrl(const QString &url)
{
    m_editBaseUrl->setText(url);
}

void ApiConfigWidget::setModel(const QString &model)
{
    int index = m_comboModel->findText(model);
    if (index >= 0) {
        m_comboModel->setCurrentIndex(index);
    }
}

QString ApiConfigWidget::apiKey() const
{
    return m_editApiKey->text();
}

QString ApiConfigWidget::baseUrl() const
{
    return m_editBaseUrl->text();
}

QString ApiConfigWidget::model() const
{
    return m_comboModel->currentText();
}

void ApiConfigWidget::onFormatChanged(int index)
{
    Q_UNUSED(index);
    // Could adjust UI based on format if needed
}

void ApiConfigWidget::onProviderChanged(int index)
{
    QString providerKey = m_comboProvider->itemData(index).toString();

    if (PROVIDERS.contains(providerKey)) {
        const ProviderConfig &config = PROVIDERS[providerKey];
        m_editBaseUrl->setText(config.baseUrl);

        m_comboModel->clear();
        for (const QString &model : config.models) {
            m_comboModel->addItem(model);
        }
    }

    // Enable/disable URL editing for custom provider
    m_editBaseUrl->setEnabled(providerKey == "custom");
}

void ApiConfigWidget::onTestConnection()
{
    QString apiKey = m_editApiKey->text();
    QString baseUrl = m_editBaseUrl->text();

    if (apiKey.isEmpty()) {
        QMessageBox::warning(this, tr("错误"), tr("请输入API Key"));
        return;
    }

    m_btnTest->setEnabled(false);
    m_btnTest->setText(tr("测试中..."));

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    QString format = m_comboFormat->currentData().toString();
    QString url = baseUrl;

    if (format == "claude") {
        url += "/v1/messages";
    } else {
        url += "/v1/chat/completions";
    }

    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());

    // Send a minimal request to test connection
    QJsonObject requestBody;
    requestBody["model"] = m_comboModel->currentText();
    requestBody["max_tokens"] = 10;

    if (format == "claude") {
        requestBody["messages"] = QJsonArray{QJsonObject{{"role", "user"}, {"content", "Hi"}}};
    } else {
        requestBody["messages"] = QJsonArray{QJsonObject{{"role", "user"}, {"content", "Hi"}}};
    }

    QNetworkReply *reply = manager->post(request, QJsonDocument(requestBody).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        m_btnTest->setEnabled(true);
        m_btnTest->setText(tr("测试连接"));

        if (reply->error() == QNetworkReply::NoError) {
            QMessageBox::information(this, tr("成功"), tr("连接成功！"));
        } else {
            QMessageBox::warning(this, tr("失败"),
                tr("连接失败: %1").arg(reply->errorString()));
        }

        reply->deleteLater();
    });
}
