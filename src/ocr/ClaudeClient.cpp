#include "ClaudeClient.h"
#include "../utils/OcrParser.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QBuffer>
#include <QRegularExpression>

namespace {

QString buildMessagesUrl(QString baseUrl)
{
    baseUrl = baseUrl.trimmed();
    while (baseUrl.endsWith('/')) {
        baseUrl.chop(1);
    }

    if (baseUrl.endsWith(QStringLiteral("/messages"))) {
        return baseUrl;
    }

    const QRegularExpression versionSuffix(QStringLiteral(R"(/v\d+$)"));
    if (versionSuffix.match(baseUrl).hasMatch()) {
        return baseUrl + QStringLiteral("/messages");
    }

    return baseUrl + QStringLiteral("/v1/messages");
}

}

ClaudeClient::ClaudeClient(QObject *parent)
    : OcrInterface(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_baseUrl("https://api.anthropic.com")
    , m_model("claude-3-5-sonnet-20241022")
{
}

void ClaudeClient::setApiKey(const QString &key)
{
    m_apiKey = key;
}

void ClaudeClient::setBaseUrl(const QString &url)
{
    m_baseUrl = url;
}

void ClaudeClient::setModel(const QString &model)
{
    m_model = model;
}

bool ClaudeClient::isReady() const
{
    return !m_apiKey.isEmpty() && !m_baseUrl.isEmpty() && !m_model.isEmpty();
}

QString ClaudeClient::encodeImageToBase64(const QImage &image)
{
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    buffer.close();

    return byteArray.toBase64();
}

void ClaudeClient::recognize(const QImage &image, const QString &prompt)
{
    if (!isReady()) {
        emit recognitionError(tr("Claude客户端未配置"));
        return;
    }

    saveRequestContext(image, prompt);
    sendRequest();
}

void ClaudeClient::sendRequest()
{
    QString base64Image = encodeImageToBase64(m_lastImage);

    // Build request body (Claude format)
    QJsonObject requestBody;
    requestBody["model"] = m_model;
    requestBody["max_tokens"] = 4096;

    QJsonObject message;
    message["role"] = "user";

    QJsonArray content;

    // Image content
    QJsonObject imageContent;
    imageContent["type"] = "image";
    QJsonObject imageSource;
    imageSource["type"] = "base64";
    imageSource["media_type"] = "image/png";
    imageSource["data"] = base64Image;
    imageContent["source"] = imageSource;
    content.append(imageContent);

    // Text content
    QJsonObject textContent;
    textContent["type"] = "text";
    textContent["text"] = m_lastPrompt;
    content.append(textContent);

    message["content"] = content;

    QJsonArray messages;
    messages.append(message);
    requestBody["messages"] = messages;

    // Send request
    const QString url = buildMessagesUrl(m_baseUrl);
    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("x-api-key", m_apiKey.toUtf8());
    request.setRawHeader("anthropic-version", "2023-06-01");
    request.setTransferTimeout(m_timeout);

    QNetworkReply *reply = m_networkManager->post(
        request,
        QJsonDocument(requestBody).toJson()
    );

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleResponse(reply);
    });
}

void ClaudeClient::handleResponse(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        if (shouldRetry("ClaudeClient:")) {
            sendRequest();
        } else {
            emit recognitionError(tr("API请求失败: %1").arg(reply->errorString()));
        }
        return;
    }

    QByteArray response = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(response);

    if (doc.isNull() || !doc.isObject()) {
        emit recognitionError(tr("API响应格式错误"));
        return;
    }

    QJsonObject result = doc.object();

    if (result.contains("error")) {
        const QJsonValue errVal = result["error"];
        if (errVal.isString()) {
            emit recognitionError(errVal.toString());
        } else if (errVal.isObject()) {
            QString msg = errVal.toObject().value("message").toString();
            if (msg.isEmpty()) msg = errVal.toObject().value("msg").toString();
            emit recognitionError(msg.isEmpty() ? tr("API错误") : msg);
        } else {
            emit recognitionError(tr("API错误"));
        }
        return;
    }

    // Extract content from Claude response
    QJsonObject output;
    if (result.contains("content")) {
        QJsonArray content = result["content"].toArray();
        QString text;
        for (const auto &item : content) {
            if (item.toObject()["type"].toString() == "text") {
                text += item.toObject()["text"].toString();
            }
        }

        // Try to parse as JSON
        QJsonDocument textDoc = OcrParser::tryParseJson(text);
        if (!textDoc.isNull()) {
            if (textDoc.isObject()) {
                output = textDoc.object();
            } else if (textDoc.isArray()) {
                output["tables"] = textDoc.array();
            }
        } else {
            output["text"] = text;
        }
    }

    emit recognitionFinished(output);
}
