#ifndef PADDLEOCR_H
#define PADDLEOCR_H

#include "OcrInterface.h"
#include "OcrServerManager.h"
#include <QNetworkAccessManager>
#include <QBuffer>

class PaddleOcr : public OcrInterface
{
    Q_OBJECT

public:
    explicit PaddleOcr(QObject *parent = nullptr);

    void setBaseUrl(const QString &url) override;
    void recognize(const QImage &image, const QString &prompt) override;
    bool isReady() const override;

    // Server management
    void setServerPath(const QString &pythonPath, const QString &scriptPath);
    void startServer();
    void stopServer();
    OcrServerManager::ServerStatus serverStatus() const;
    bool isServerRunning() const;

signals:
    void serverStatusChanged(OcrServerManager::ServerStatus status);

private:
    bool ensureServerRunning();
    void sendRequest(const QByteArray &imageData, const QString &prompt);
    void handleResponse(QNetworkReply *reply);

    QNetworkAccessManager *m_networkManager;
    OcrServerManager *m_serverManager;
    QString m_serverUrl;
    QBuffer m_buffer;
};

#endif // PADDLEOCR_H
