#include "OpenAIClient.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QBuffer>

namespace {
QJsonDocument tryParseJson(QString text)
{
    text = text.trimmed();
    if (text.startsWith("```")) {
        int firstNewline = text.indexOf('\n');
        if (firstNewline >= 0) {
            text = text.mid(firstNewline + 1);
        }
        if (text.endsWith("```")) {
            text.chop(3);
        }
        text = text.trimmed();
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8(), &err);
    if (!doc.isNull()) {
        return doc;
    }

    int firstBrace = text.indexOf('{');
    int lastBrace = text.lastIndexOf('}');
    if (firstBrace >= 0 && lastBrace > firstBrace) {
        doc = QJsonDocument::fromJson(text.mid(firstBrace, lastBrace - firstBrace + 1).toUtf8(), &err);
        if (!doc.isNull()) {
            return doc;
        }
    }

    int firstBracket = text.indexOf('[');
    int lastBracket = text.lastIndexOf(']');
    if (firstBracket >= 0 && lastBracket > firstBracket) {
        doc = QJsonDocument::fromJson(text.mid(firstBracket, lastBracket - firstBracket + 1).toUtf8(), &err);
        if (!doc.isNull()) {
            return doc;
        }
    }

    return QJsonDocument();
}
}

OpenAIClient::OpenAIClient(QObject *parent)
    : OcrInterface(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_baseUrl("https://api.deepseek.com")
    , m_model("deepseek-chat")
{
}

void OpenAIClient::setApiKey(const QString &key)
{
    m_apiKey = key;
}

void OpenAIClient::setBaseUrl(const QString &url)
{
    m_baseUrl = url;
}

void OpenAIClient::setModel(const QString &model)
{
    m_model = model;
}

bool OpenAIClient::isReady() const
{
    return !m_apiKey.isEmpty() && !m_baseUrl.isEmpty() && !m_model.isEmpty();
}

QString OpenAIClient::encodeImageToBase64(const QImage &image)
{
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    buffer.close();

    return "data:image/png;base64," + byteArray.toBase64();
}

void OpenAIClient::recognize(const QImage &image, const QString &prompt)
{
    if (!isReady()) {
        emit recognitionError(tr("OpenAI客户端未配置"));
        return;
    }

    QString base64Image = encodeImageToBase64(image);

    // Build request body (OpenAI format)
    QJsonObject requestBody;
    requestBody["model"] = m_model;
    requestBody["max_tokens"] = 4096;

    QJsonArray messages;

    QJsonObject userMessage;
    userMessage["role"] = "user";

    QJsonArray content;

    // Image content
    QJsonObject imageContent;
    imageContent["type"] = "image_url";
    QJsonObject imageUrl;
    imageUrl["url"] = base64Image;
    imageContent["image_url"] = imageUrl;
    content.append(imageContent);

    // Text content
    QJsonObject textContent;
    textContent["type"] = "text";
    textContent["text"] = prompt;
    content.append(textContent);

    userMessage["content"] = content;
    messages.append(userMessage);

    requestBody["messages"] = messages;

    // Send request
    QString url = m_baseUrl + "/v1/chat/completions";
    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());

    QNetworkReply *reply = m_networkManager->post(
        request,
        QJsonDocument(requestBody).toJson()
    );

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleResponse(reply);
    });
}

void OpenAIClient::handleResponse(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit recognitionError(tr("API请求失败: %1").arg(reply->errorString()));
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
        emit recognitionError(result["error"].toObject()["message"].toString());
        return;
    }

    // Extract content from OpenAI response
    QJsonObject output;
    if (result.contains("choices")) {
        QJsonArray choices = result["choices"].toArray();
        if (!choices.isEmpty()) {
            QString text = choices[0].toObject()["message"].toObject()["content"].toString();

            // Try to parse as JSON
            QJsonDocument textDoc = tryParseJson(text);
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
    }

    emit recognitionFinished(output);
}
