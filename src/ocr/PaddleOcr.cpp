#include "PaddleOcr.h"
#include "core/ConfigManager.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QBuffer>
#include <QCoreApplication>

PaddleOcr::PaddleOcr(QObject *parent)
    : OcrInterface(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_serverManager(new OcrServerManager(this))
    , m_serverUrl("http://127.0.0.1:5000")
{
    // Connect server status changes
    connect(m_serverManager, &OcrServerManager::statusChanged,
            this, &PaddleOcr::serverStatusChanged);
    connect(m_serverManager, &OcrServerManager::serverError,
            this, [this](const QString &msg) {
        emit recognitionError(msg);
    });

    // Set default server configuration from ConfigManager
    QString pythonPath = ConfigManager::instance()->pythonPath();
    QString scriptPath = ConfigManager::instance()->ocrServerPath();
    m_serverManager->setServerPath(pythonPath, scriptPath);
    m_serverManager->setServerUrl(m_serverUrl);
}

void PaddleOcr::setBaseUrl(const QString &url)
{
    m_serverUrl = url;
    m_serverManager->setServerUrl(url);
}

bool PaddleOcr::isReady() const
{
    return m_serverManager->isRunning();
}

void PaddleOcr::setServerPath(const QString &pythonPath, const QString &scriptPath)
{
    m_serverManager->setServerPath(pythonPath, scriptPath);
}

void PaddleOcr::startServer()
{
    m_serverManager->start();
}

void PaddleOcr::stopServer()
{
    m_serverManager->stop();
}

OcrServerManager::ServerStatus PaddleOcr::serverStatus() const
{
    return m_serverManager->status();
}

bool PaddleOcr::isServerRunning() const
{
    return m_serverManager->isRunning();
}

bool PaddleOcr::ensureServerRunning()
{
    if (m_serverManager->isRunning()) {
        return true;
    }

    // Start server if not running
    if (m_serverManager->status() == OcrServerManager::NotRunning ||
        m_serverManager->status() == OcrServerManager::Error) {
        m_serverManager->start();
    }

    // Server is starting, will be ready when status changes to Running
    return false;
}

void PaddleOcr::recognize(const QImage &image, const QString &prompt)
{
    // Check if server is ready, if not start it
    if (!m_serverManager->isRunning()) {
        if (m_serverManager->status() == OcrServerManager::NotRunning ||
            m_serverManager->status() == OcrServerManager::Error) {
            m_serverManager->start();
            emit recognitionError(tr("OCR服务器正在启动，请稍后重试"));
            return;
        } else if (m_serverManager->status() == OcrServerManager::Starting) {
            emit recognitionError(tr("OCR服务器正在启动中，请稍后重试"));
            return;
        }
    }

    // Convert image to base64
    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    buffer.close();

    QString base64Image = QString("data:image/png;base64,") + imageData.toBase64();

    // Create JSON request
    QJsonObject json;
    json["image"] = base64Image;

    QJsonObject options;
    const bool detectTable = prompt.contains(QStringLiteral("表格"), Qt::CaseInsensitive)
        || prompt.contains(QStringLiteral("table"), Qt::CaseInsensitive);
    options["detect_table"] = detectTable;
    options["return_boxes"] = false;
    json["options"] = options;

    QJsonDocument doc(json);

    // Send request
    QNetworkRequest request(QUrl(m_serverUrl + "/ocr"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = m_networkManager->post(request, doc.toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleResponse(reply);
    });
}

void PaddleOcr::handleResponse(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit recognitionError(tr("OCR请求失败: %1").arg(reply->errorString()));
        return;
    }

    QByteArray response = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(response);

    if (doc.isNull() || !doc.isObject()) {
        emit recognitionError(tr("OCR响应格式错误"));
        return;
    }

    QJsonObject result = doc.object();

    if (result.contains("error")) {
        emit recognitionError(result["error"].toString());
        return;
    }

    emit recognitionFinished(result);
}
