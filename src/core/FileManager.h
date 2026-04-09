#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QObject>
#include <QStringList>
#include <QImage>

class FileConverter;

class FileManager : public QObject
{
    Q_OBJECT

public:
    explicit FileManager(QObject *parent = nullptr);

    bool addFile(const QString &filePath);
    void addFiles(const QStringList &filePaths);
    void removeFile(const QString &filePath);
    void clear();

    QStringList files() const;
    QStringList supportedExtensions() const;
    bool isFileSupported(const QString &filePath) const;

    QImage loadImage(const QString &filePath);
    bool convertToImage(const QString &filePath, QImage &outImage);

signals:
    void fileAdded(const QString &filePath);
    void fileRemoved(const QString &filePath);
    void filesChanged();

private:
    QString detectFileType(const QString &filePath) const;

    QStringList m_files;
    FileConverter *m_converter;

    static const QStringList SUPPORTED_EXTENSIONS;
};

#endif // FILEMANAGER_H
