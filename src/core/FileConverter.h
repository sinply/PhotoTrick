#ifndef FILECONVERTER_H
#define FILECONVERTER_H

#include <QObject>
#include <QImage>
#include <QProcess>

class FileConverter : public QObject
{
    Q_OBJECT

public:
    explicit FileConverter(QObject *parent = nullptr);

    bool convertToImage(const QString &filePath, QImage &outImage);
    QList<QImage> convertToImages(const QString &filePath);

    bool convertHeic(const QString &filePath, QImage &outImage);
    bool convertPdf(const QString &filePath, QList<QImage> &outImages);
    bool convertOfd(const QString &filePath, QList<QImage> &outImages);
    bool extractImagesFromDocx(const QString &filePath, QList<QImage> &outImages);
    bool extractImagesFromXlsx(const QString &filePath, QList<QImage> &outImages);

    void setPythonPath(const QString &path);
    void setConverterScriptPath(const QString &path);

private:
    bool runPythonConverter(const QString &action, const QString &filePath, const QString &outputPath);

    QString m_pythonPath;
    QString m_scriptPath;
};

#endif // FILECONVERTER_H
