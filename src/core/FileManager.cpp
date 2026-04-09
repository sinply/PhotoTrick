#include "FileManager.h"
#include "FileConverter.h"
#include <QFileInfo>
#include <QImageReader>

const QStringList FileManager::SUPPORTED_EXTENSIONS = {
    "jpg", "jpeg", "png", "bmp", "gif", "tiff", "webp", "heic",
    "pdf", "ofd", "docx", "xlsx"
};

FileManager::FileManager(QObject *parent)
    : QObject(parent)
    , m_converter(new FileConverter(this))
{
}

bool FileManager::addFile(const QString &filePath)
{
    if (!isFileSupported(filePath)) {
        return false;
    }

    if (!m_files.contains(filePath)) {
        m_files.append(filePath);
        emit fileAdded(filePath);
        emit filesChanged();
        return true;
    }

    return false;
}

void FileManager::addFiles(const QStringList &filePaths)
{
    for (const QString &path : filePaths) {
        addFile(path);
    }
}

void FileManager::removeFile(const QString &filePath)
{
    if (m_files.removeOne(filePath)) {
        emit fileRemoved(filePath);
        emit filesChanged();
    }
}

void FileManager::clear()
{
    m_files.clear();
    emit filesChanged();
}

QStringList FileManager::files() const
{
    return m_files;
}

QStringList FileManager::supportedExtensions() const
{
    return SUPPORTED_EXTENSIONS;
}

bool FileManager::isFileSupported(const QString &filePath) const
{
    QString ext = QFileInfo(filePath).suffix().toLower();
    return SUPPORTED_EXTENSIONS.contains(ext);
}

QImage FileManager::loadImage(const QString &filePath)
{
    QString fileType = detectFileType(filePath);

    // Direct image formats
    if (fileType == "image") {
        QImageReader reader(filePath);
        reader.setAutoTransform(true);
        return reader.read();
    }

    // Convert other formats
    QImage converted;
    if (convertToImage(filePath, converted)) {
        return converted;
    }

    return QImage();
}

bool FileManager::convertToImage(const QString &filePath, QImage &outImage)
{
    QString fileType = detectFileType(filePath);

    if (fileType == "image") {
        outImage = loadImage(filePath);
        return !outImage.isNull();
    }

    if (m_converter) {
        return m_converter->convertToImage(filePath, outImage);
    }

    return false;
}

QString FileManager::detectFileType(const QString &filePath) const
{
    QString ext = QFileInfo(filePath).suffix().toLower();

    if (SUPPORTED_EXTENSIONS.mid(0, 8).contains(ext)) {
        return "image";
    }
    if (ext == "pdf") return "pdf";
    if (ext == "ofd") return "ofd";
    if (ext == "docx") return "docx";
    if (ext == "xlsx") return "xlsx";

    return "unknown";
}
