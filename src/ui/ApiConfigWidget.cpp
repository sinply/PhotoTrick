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
    bool enabled;
};

static const QMap<QString, ProviderConfig> PROVIDERS = {
    {"deepseek", {"DeepSeek", "https://api.deepseek.com", {"deepseek-chat"}, true}},
    {"glm", {"GLM (智谱)", "https://open.bigmodel.cn/api/paas/v4", {"glm-4v"}, true}},
    {"qwen", {"通义千问 (暂不支持)", "https://dashscope.aliyuncs.com/compatible-mode", {"qwen-vl-plus"}, false}},
    {"moonshot", {"月之暗面 (暂不支持)", "https://api.moonshot.cn", {"moonshot-v1-8k-vision"}, false}},
    {"custom", {"自定义", "", {}, true}}
};

ApiConfigWidget::ApiConfigWidget(QWidget *parent)
    : QDialog(parent)
    , m_comboFormat(nullptr)
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
        int index = m_comboProvider->count() - 1;
        if (!it.value().enabled) {
            // 设置禁用项的样式为灰色
            m_comboProvider->setItemData(index, QColor(128, 128, 128), Qt::ForegroundRole);
            m_comboProvider->setItemData(index, false, Qt::UserRole - 1); // 禁用标志
        }
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

    // Model - 使用可编辑的组合框
    m_comboModel = new QComboBox(this);
    m_comboModel->setEditable(true);  // 允许编辑
    m_comboModel->setInsertPolicy(QComboBox::InsertAtTop);
    m_editModel = m_comboModel->lineEdit();  // 获取内部的LineEdit
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
    } else {
        m_comboModel->setEditText(model);
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

void ApiConfigWidget::onFormatChanged(int index)
{
    Q_UNUSED(index);
    // Could adjust UI based on format if needed
}

void ApiConfigWidget::onProviderChanged(int index)
{
    QString providerKey = m_comboProvider->itemData(index).toString();

    // 检查是否禁用的服务商
    QVariant disabledFlag = m_comboProvider->itemData(index, Qt::UserRole - 1);
    if (disabledFlag.isValid() && !disabledFlag.toBool()) {
        // 找到第一个可用的服务商并切换
        for (int i = 0; i < m_comboProvider->count(); ++i) {
            QString key = m_comboProvider->itemData(i).toString();
            if (PROVIDERS.contains(key) && PROVIDERS[key].enabled) {
                m_comboProvider->setCurrentIndex(i);
                return;
            }
        }
        return;
    }

    if (PROVIDERS.contains(providerKey)) {
        const ProviderConfig &config = PROVIDERS[providerKey];
        m_editBaseUrl->setText(config.baseUrl);

        m_comboModel->clear();
        for (const QString &model : config.models) {
            m_comboModel->addItem(model);
        }
    }

    // Base URL始终可编辑，让用户可以自定义
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
    requestBody["model"] = model;
    requestBody["max_tokens"] = 10;

    if (format == "claude") {
        requestBody["messages"] = QJsonArray{QJsonObject{{"role", "user"}, {"content", "Hi"}}};
    } else {
        requestBody["messages"] = QJsonArray{QJsonObject{{"role", "user"}, {"content", "Hi"}}};
    }

    QNetworkReply *reply = manager->post(request, QJsonDocument(requestBody).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, manager]() {
        m_btnTest->setEnabled(true);
        m_btnTest->setText(tr("测试连接"));

        if (reply->error() == QNetworkReply::NoError) {
            setApiStatus(ApiStatus::Valid, tr("API状态: 连接成功 ✓"));
            QMessageBox::information(this, tr("成功"), tr("连接成功！API Key有效。"));
        } else {
            QString errorMsg = reply->errorString();
            // 尝试解析错误详情
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
