#ifndef OCRMANAGER_H
#define OCRMANAGER_H

#include <QObject>
#include <QImage>
#include <QJsonObject>

class OcrInterface;
class PaddleOcr;

class OcrManager : public QObject
{
    Q_OBJECT

public:
    enum Backend {
        PaddleOCR_Local,
        Claude_Format,
        OpenAI_Format
    };

    explicit OcrManager(QObject *parent = nullptr);

    void setBackend(Backend backend);
    Backend backend() const;

    void setApiKey(const QString &key);
    void setBaseUrl(const QString &url);
    void setModel(const QString &model);

    void recognizeImage(const QImage &image, const QString &prompt);
    void recognizeImages(const QList<QImage> &images, const QString &prompt);

    PaddleOcr* paddleOcrClient() const;

signals:
    void recognitionStarted();
    void recognitionFinished(const QJsonObject &result);
    void recognitionError(const QString &error);
    void progress(int percent);
    void serverStatusChanged(int status);

private:
    Backend m_backend;
    QString m_apiKey;
    QString m_baseUrl;
    QString m_model;
    OcrInterface *m_currentClient;
};

#endif // OCRMANAGER_H
