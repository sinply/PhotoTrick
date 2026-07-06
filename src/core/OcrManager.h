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

    // 中断当前正在进行的 OCR 请求（用于取消处理）
    void cancelCurrent();

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

    // 多图串行队列：上一张 finished/error 后再发下一张，避免覆盖未完成请求
    QList<QImage> m_pendingImages;
    QString m_pendingPrompt;
    int m_pendingTotal = 0;
    int m_pendingIndex = 0;
    bool m_batchActive = false;

private slots:
    void onBatchFinished(const QJsonObject &result);
    void onBatchError(const QString &error);
    void dispatchNextPending();
};

#endif // OCRMANAGER_H
