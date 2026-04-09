#include "PaddleOcr.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QBuffer>

PaddleOcr::PaddleOcr(QObject *parent)
    : OcrInterface(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_serverUrl("http://localhost:5000")
{
}

void PaddleOcr::setBaseUrl(const QString &url)
{
    m_serverUrl = url;
}

bool PaddleOcr::isReady() const
{
    return !m_serverUrl.isEmpty();
}

void PaddleOcr::recognize(const QImage &image, const QString &prompt)
{
    Q_UNUSED(prompt)

    if (!isReady()) {
        emit recognitionError(tr("PaddleOCR服务器未配置"));
        return;
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
    options["detect_table"] = false;
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
