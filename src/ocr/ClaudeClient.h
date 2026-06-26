#ifndef CLAUDECLIENT_H
#define CLAUDECLIENT_H

#include "OcrInterface.h"
#include <QNetworkAccessManager>
#include <QPointer>

class ClaudeClient : public OcrInterface
{
    Q_OBJECT

public:
    explicit ClaudeClient(QObject *parent = nullptr);

    void setApiKey(const QString &key) override;
    void setBaseUrl(const QString &url) override;
    void setModel(const QString &model) override;

    void recognize(const QImage &image, const QString &prompt) override;
    bool isReady() const override;
    void cancel() override;

private:
    QString encodeImageToBase64(const QImage &image);
    void sendRequest();
    void handleResponse(QNetworkReply *reply);

    QNetworkAccessManager *m_networkManager;
    QPointer<QNetworkReply> m_currentReply;
    QString m_apiKey;
    QString m_baseUrl;
    QString m_model;
};

#endif // CLAUDECLIENT_H
