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

signals:
    void recognitionFinished(const QJsonObject &result);
    void recognitionError(const QString &error);
    void progress(int percent);
};

#endif // OCRINTERFACE_H
