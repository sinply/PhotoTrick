#ifndef ITINERARYRECOGNIZER_H
#define ITINERARYRECOGNIZER_H

#include <QObject>
#include <QImage>
#include <QJsonObject>
#include "../models/ItineraryData.h"

class OcrManager;

class ItineraryRecognizer : public QObject
{
    Q_OBJECT

public:
    explicit ItineraryRecognizer(QObject *parent = nullptr);

    void recognize(const QImage &image);
    void recognizeAsync(const QImage &image);

    void setOcrManager(OcrManager *manager);

signals:
    void recognitionFinished(const ItineraryData &itinerary);
    void recognitionError(const QString &error);
    void rawOcrReceived(const QJsonObject &result);

private slots:
    void onOcrFinished(const QJsonObject &result);
    void onOcrError(const QString &error);

private:
    ItineraryData parseItineraryData(const QJsonObject &json);

    OcrManager *m_ocrManager;
    bool m_isRecognizing = false;

    // Prompt template for itinerary recognition
    static const QString ITINERARY_PROMPT;
};

#endif // ITINERARYRECOGNIZER_H
