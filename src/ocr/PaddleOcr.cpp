#include "PaddleOcr.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QBuffer>

PaddleOcr::PaddleOcr(QObject *parent)
    : OcrInterface(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_serverUrl("http://localhost:8080")
{
}

void PaddleOcr::setServerUrl(const QString &url)
{
    m_serverUrl = url;
}

QString PaddleOcr::serverUrl() const
{
    return m_serverUrl;
}

bool PaddleOcr::isReady() const
{
    return !m_serverUrl.isEmpty();
}

void PaddleOcr::recognize(const QImage &image, const QString &prompt)
{
    if (!isReady()) {
        emit recognitionError(tr("PaddleOCR服务器未配置"));
        return;
    }

    // Convert image to byte array
    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    buffer.close();

    // Create multipart request
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, "image/png");
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QVariant("form-data; name=\"image\"; filename=\"image.png\""));
    imagePart.setBody(imageData);
    multiPart->append(imagePart);

    QHttpPart promptPart;
    promptPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                         QVariant("form-data; name=\"prompt\""));
    promptPart.setBody(prompt.toUtf8());
    multiPart->append(promptPart);

    // Send request
    QNetworkRequest request(QUrl(m_serverUrl + "/ocr"));
    QNetworkReply *reply = m_networkManager->post(request, multiPart);
    multiPart->setParent(reply);

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
