#include "ApiConfigWidget.h"
#include "../core/ConfigManager.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// Provider configurations
struct ProviderConfig {
    QString name;
    QString baseUrl;
    QStringList models;
    bool enabled;
};

static const QMap<QString, ProviderConfig> PROVIDERS = {
    {"deepseek", {"DeepSeek", "https://api.deepseek.com", {"deepseek-chat"}, true}},
    {"glm", {"GLM (智谱)", "https://open.bigmodel.cn/api/paas/v4", {"glm-4v"}, true}},
    {"custom", {"自定义", "", {}, true}}
};

ApiConfigWidget::ApiConfigWidget(const QString &backend, QWidget *parent)
    : QDialog(parent)
    , m_backend(backend)
    , m_comboProvider(nullptr)
    , m_editApiKey(nullptr)
    , m_editBaseUrl(nullptr)
    , m_comboModel(nullptr)
    , m_editModel(nullptr)
    , m_btnTest(nullptr)
    , m_labelApiStatus(nullptr)
{
    setupUI();
    setupConnections();
    loadSettings();
}

void ApiConfigWidget::setupUI()
{
    setWindowTitle(tr("API配置 - %1").arg(m_backend));
    setMinimumSize(400, 280);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QFormLayout *form = new QFormLayout();

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

    // Model - 可编辑的组合框
    m_comboModel = new QComboBox(this);
    m_comboModel->setEditable(true);
    m_comboModel->setInsertPolicy(QComboBox::InsertAtTop);
    m_editModel = m_comboModel->lineEdit();
    form->addRow(tr("模型:"), m_comboModel);

    // API Status
    m_labelApiStatus = new QLabel(tr("API状态: 未配置"), this);
    m_labelApiStatus->setStyleSheet("QLabel { color: gray; }");
    form->addRow(QString(), m_labelApiStatus);

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

    connect(btnSave, &QPushButton::clicked, this, &ApiConfigWidget::onSave);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

void ApiConfigWidget::setupConnections()
{
    connect(m_comboProvider, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ApiConfigWidget::onProviderChanged);
    connect(m_btnTest, &QPushButton::clicked, this, &ApiConfigWidget::onTestConnection);
}

void ApiConfigWidget::loadSettings()
{
    ConfigManager *cfg = ConfigManager::instance();

    // Load per-backend settings
    QString savedProvider = cfg->value(QString("providers/%1").arg(m_backend)).toString();
    QString savedApiKey = cfg->apiKey(m_backend);
    QString savedBaseUrl = cfg->baseUrl(m_backend);
    QString savedModel = cfg->model(m_backend);

    // Block signals to prevent onProviderChanged from overwriting saved values
    m_comboProvider->blockSignals(true);

    // Set provider
    if (!savedProvider.isEmpty()) {
        int index = m_comboProvider->findData(savedProvider);
        if (index >= 0) {
            m_comboProvider->setCurrentIndex(index);
        }
    }

    // Populate models for current provider (without triggering signal)
    QString providerKey = m_comboProvider->currentData().toString();
    if (PROVIDERS.contains(providerKey)) {
        m_comboModel->clear();
        for (const QString &model : PROVIDERS[providerKey].models) {
            m_comboModel->addItem(model);
        }
    }

    m_comboProvider->blockSignals(false);

    // Set API key
    m_editApiKey->setText(savedApiKey);

    // Set base URL - saved value takes priority over provider default
    if (!savedBaseUrl.isEmpty()) {
        m_editBaseUrl->setText(savedBaseUrl);
    } else if (PROVIDERS.contains(providerKey)) {
        m_editBaseUrl->setText(PROVIDERS[providerKey].baseUrl);
    }

    // Set model
    if (!savedModel.isEmpty()) {
        m_comboModel->setEditText(savedModel);
    }

    // Update status
    if (!savedApiKey.isEmpty()) {
        setApiStatus(ApiStatus::Configured);
    }
}

void ApiConfigWidget::saveSettings()
{
    ConfigManager *cfg = ConfigManager::instance();

    // Save per-backend settings
    cfg->setValue(QString("providers/%1").arg(m_backend), m_comboProvider->currentData().toString());
    cfg->setApiKey(m_backend, m_editApiKey->text());
    cfg->setBaseUrl(m_backend, m_editBaseUrl->text());
    cfg->setModel(m_backend, m_comboModel->currentText().trimmed());
    cfg->save();
}

void ApiConfigWidget::onSave()
{
    saveSettings();
    accept();
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
    m_comboModel->setEditText(model);
}

void ApiConfigWidget::setProvider(const QString &provider)
{
    int index = m_comboProvider->findData(provider);
    if (index >= 0) {
        m_comboProvider->setCurrentIndex(index);
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
    return m_comboModel->currentText().trimmed();
}

QString ApiConfigWidget::provider() const
{
    return m_comboProvider->currentData().toString();
}

void ApiConfigWidget::setApiStatus(ApiStatus status, const QString &message)
{
    QString text;
    QString style;

    switch (status) {
    case ApiStatus::NotConfigured:
        text = tr("API状态: 未配置");
        style = "QLabel { color: gray; }";
        break;
    case ApiStatus::Configured:
        text = tr("API状态: 已配置");
        style = "QLabel { color: #0066cc; }";
        break;
    case ApiStatus::Testing:
        text = tr("API状态: 测试中...");
        style = "QLabel { color: orange; }";
        break;
    case ApiStatus::Valid:
        text = message.isEmpty() ? tr("API状态: 有效 ✓") : message;
        style = "QLabel { color: green; font-weight: bold; }";
        break;
    case ApiStatus::Invalid:
        text = message.isEmpty() ? tr("API状态: 无效 ✗") : message;
        style = "QLabel { color: red; }";
        break;
    }

    m_labelApiStatus->setText(text);
    m_labelApiStatus->setStyleSheet(style);
}

void ApiConfigWidget::onProviderChanged(int index)
{
    QString providerKey = m_comboProvider->itemData(index).toString();

    if (PROVIDERS.contains(providerKey)) {
        const ProviderConfig &config = PROVIDERS[providerKey];

        // Populate models
        m_comboModel->clear();
        for (const QString &model : config.models) {
            m_comboModel->addItem(model);
        }

        // Auto-fill base URL only if empty (user hasn't entered anything)
        if (m_editBaseUrl->text().trimmed().isEmpty()) {
            m_editBaseUrl->setText(config.baseUrl);
        }
    }

    // Base URL always editable
    m_editBaseUrl->setEnabled(true);
}

void ApiConfigWidget::onTestConnection()
{
    QString apiKey = m_editApiKey->text();
    QString baseUrl = m_editBaseUrl->text();
    QString model = m_comboModel->currentText().trimmed();

    if (apiKey.isEmpty()) {
        QMessageBox::warning(this, tr("错误"), tr("请输入API Key"));
        return;
    }

    if (model.isEmpty()) {
        QMessageBox::warning(this, tr("错误"), tr("请输入模型名称"));
        return;
    }

    m_btnTest->setEnabled(false);
    m_btnTest->setText(tr("测试中..."));
    setApiStatus(ApiStatus::Testing);

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    // Determine API endpoint based on backend type
    QString url = baseUrl;
    if (m_backend == "claude_format") {
        url += "/v1/messages";
    } else {
        url += "/v1/chat/completions";
    }

    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());

    QJsonObject requestBody;
    requestBody["model"] = model;
    requestBody["max_tokens"] = 10;
    requestBody["messages"] = QJsonArray{QJsonObject{{"role", "user"}, {"content", "Hi"}}};

    QNetworkReply *reply = manager->post(request, QJsonDocument(requestBody).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, manager]() {
        m_btnTest->setEnabled(true);
        m_btnTest->setText(tr("测试连接"));

        if (reply->error() == QNetworkReply::NoError) {
            setApiStatus(ApiStatus::Valid, tr("API状态: 连接成功 ✓"));
            QMessageBox::information(this, tr("成功"), tr("连接成功！API Key有效。"));
        } else {
            QString errorMsg = reply->errorString();
            QByteArray responseData = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                if (obj.contains("error")) {
                    QJsonObject error = obj["error"].toObject();
                    if (error.contains("message")) {
                        errorMsg = error["message"].toString();
                    }
                }
            }
            setApiStatus(ApiStatus::Invalid, tr("API状态: %1").arg(errorMsg.left(30)));
            QMessageBox::warning(this, tr("失败"), tr("连接失败: %1").arg(errorMsg));
        }

        reply->deleteLater();
        manager->deleteLater();
    });
}
