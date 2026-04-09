#ifndef CLAUDECLIENT_H
#define CLAUDECLIENT_H

#include "OcrInterface.h"
#include <QNetworkAccessManager>

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

private:
    QString encodeImageToBase64(const QImage &image);
    void handleResponse(QNetworkReply *reply);

    QNetworkAccessManager *m_networkManager;
    QString m_apiKey;
    QString m_baseUrl;
    QString m_model;
};

#endif // CLAUDECLIENT_H
