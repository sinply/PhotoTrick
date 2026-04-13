#ifndef TABLEEXTRACTOR_H
#define TABLEEXTRACTOR_H

#include <QObject>
#include <QImage>
#include <QJsonObject>
#include "../models/TableData.h"

class OcrManager;

class TableExtractor : public QObject
{
    Q_OBJECT

public:
    explicit TableExtractor(QObject *parent = nullptr);

    void extract(const QImage &image);
    void extractAsync(const QImage &image);

    void setOcrManager(OcrManager *manager);

signals:
    void extractionFinished(const TableData &table);
    void extractionError(const QString &error);
    void rawOcrReceived(const QJsonObject &result);

private slots:
    void onOcrFinished(const QJsonObject &result);
    void onOcrError(const QString &error);

private:
    TableData parseTableData(const QJsonObject &json);

    OcrManager *m_ocrManager;
    bool m_isExtracting = false;

    // Prompt template for table extraction
    static const QString TABLE_PROMPT;
};

#endif // TABLEEXTRACTOR_H
