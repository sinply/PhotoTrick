#ifndef INVOICERECOGNIZER_H
#define INVOICERECOGNIZER_H

#include <QObject>
#include <QImage>
#include <QJsonObject>
#include <QJsonValue>
#include "../models/InvoiceData.h"

class OcrManager;

class InvoiceRecognizer : public QObject
{
    Q_OBJECT

public:
    explicit InvoiceRecognizer(QObject *parent = nullptr);

    void recognize(const QImage &image);
    void recognizeAsync(const QImage &image);

    void setOcrManager(OcrManager *manager);

signals:
    void recognitionFinished(const InvoiceData &invoice);
    void recognitionError(const QString &error);
    void rawOcrReceived(const QJsonObject &result);

private slots:
    void onOcrFinished(const QJsonObject &result);
    void onOcrError(const QString &error);

private:
    InvoiceData parseInvoiceData(const QJsonObject &json);
    double parseAmount(const QJsonValue &value);

    OcrManager *m_ocrManager;
    bool m_isRecognizing = false;

    // Prompt template for invoice recognition
    static const QString INVOICE_PROMPT;
};

#endif // INVOICERECOGNIZER_H
