#ifndef INVOICERECOGNIZER_H
#define INVOICERECOGNIZER_H

#include <QObject>
#include <QImage>
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

private slots:
    void onOcrFinished(const QJsonObject &result);

private:
    InvoiceData parseInvoiceData(const QJsonObject &json);

    OcrManager *m_ocrManager;

    // Prompt template for invoice recognition
    static const QString INVOICE_PROMPT;
};

#endif // INVOICERECOGNIZER_H
