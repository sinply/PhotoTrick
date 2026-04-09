#ifndef PADDLEOCR_H
#define PADDLEOCR_H

#include "OcrInterface.h"
#include <QNetworkAccessManager>
#include <QBuffer>

class PaddleOcr : public OcrInterface
{
    Q_OBJECT

public:
    explicit PaddleOcr(QObject *parent = nullptr);

    void recognize(const QImage &image, const QString &prompt) override;
    bool isReady() const override;

    void setServerUrl(const QString &url);
    QString serverUrl() const;

private:
    void sendRequest(const QByteArray &imageData, const QString &prompt);
    void handleResponse(QNetworkReply *reply);

    QNetworkAccessManager *m_networkManager;
    QString m_serverUrl;
    QBuffer m_buffer;
};

#endif // PADDLEOCR_H
