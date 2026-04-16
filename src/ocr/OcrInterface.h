#ifndef OCRINTERFACE_H
#define OCRINTERFACE_H

#include <QObject>
#include <QImage>
#include <QJsonObject>

class OcrInterface : public QObject
{
    Q_OBJECT

public:
    explicit OcrInterface(QObject *parent = nullptr) : QObject(parent) {}

    virtual void setApiKey(const QString &key) { Q_UNUSED(key) }
    virtual void setBaseUrl(const QString &url) { Q_UNUSED(url) }
    virtual void setModel(const QString &model) { Q_UNUSED(model) }

    virtual void recognize(const QImage &image, const QString &prompt) = 0;
    virtual bool isReady() const = 0;

    void setTimeout(int msec) { m_timeout = msec; }
    void setMaxRetries(int retries) { m_maxRetries = retries; }

protected:
    void saveRequestContext(const QImage &image, const QString &prompt)
    {
        m_lastImage = image;
        m_lastPrompt = prompt;
        m_retryCount = 0;
    }

    bool shouldRetry(const QString &logPrefix)
    {
        if (m_retryCount < m_maxRetries) {
            m_retryCount++;
            qDebug() << logPrefix << "Retry" << m_retryCount << "of" << m_maxRetries;
            return true;
        }
        return false;
    }

    int m_timeout = 60000;
    int m_maxRetries = 1;
    int m_retryCount = 0;
    QImage m_lastImage;
    QString m_lastPrompt;

signals:
    void recognitionFinished(const QJsonObject &result);
    void recognitionError(const QString &error);
    void progress(int percent);
};

#endif // OCRINTERFACE_H
