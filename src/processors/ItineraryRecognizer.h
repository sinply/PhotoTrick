#ifndef ITINERARYRECOGNIZER_H
#define ITINERARYRECOGNIZER_H

#include <QObject>
#include <QImage>
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

private slots:
    void onOcrFinished(const QJsonObject &result);

private:
    ItineraryData parseItineraryData(const QJsonObject &json);

    OcrManager *m_ocrManager;

    // Prompt template for itinerary recognition
    static const QString ITINERARY_PROMPT;
};

#endif // ITINERARYRECOGNIZER_H
