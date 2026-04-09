#ifndef TABLEEXTRACTOR_H
#define TABLEEXTRACTOR_H

#include <QObject>
#include <QImage>
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

private slots:
    void onOcrFinished(const QJsonObject &result);

private:
    TableData parseTableData(const QJsonObject &json);

    OcrManager *m_ocrManager;

    // Prompt template for table extraction
    static const QString TABLE_PROMPT;
};

#endif // TABLEEXTRACTOR_H
