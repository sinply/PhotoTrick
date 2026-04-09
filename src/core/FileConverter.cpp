#include "FileConverter.h"
#include <QFileInfo>
#include <QDir>
#include <QTemporaryFile>
#include <QDebug>

FileConverter::FileConverter(QObject *parent)
    : QObject(parent)
    , m_pythonPath("python")
    , m_scriptPath("scripts/file_converter.py")
{
}

bool FileConverter::convertToImage(const QString &filePath, QImage &outImage)
{
    QString ext = QFileInfo(filePath).suffix().toLower();

    if (ext == "heic") {
        return convertHeic(filePath, outImage);
    }

    QList<QImage> images = convertToImages(filePath);
    if (!images.isEmpty()) {
        outImage = images.first();
        return true;
    }

    return false;
}

QList<QImage> FileConverter::convertToImages(const QString &filePath)
{
    QList<QImage> images;
    QString ext = QFileInfo(filePath).suffix().toLower();

    if (ext == "pdf") {
        convertPdf(filePath, images);
    } else if (ext == "ofd") {
        convertOfd(filePath, images);
    } else if (ext == "docx") {
        extractImagesFromDocx(filePath, images);
    } else if (ext == "xlsx") {
        extractImagesFromXlsx(filePath, images);
    } else if (ext == "heic") {
        QImage img;
        if (convertHeic(filePath, img)) {
            images.append(img);
        }
    }

    return images;
}

bool FileConverter::convertHeic(const QString &filePath, QImage &outImage)
{
    // Create temporary output path
    QString outputPath = QDir::tempPath() + "/phototrick_heic_" +
                         QFileInfo(filePath).baseName() + ".jpg";

    if (runPythonConverter("heic_to_jpg", filePath, outputPath)) {
        outImage.load(outputPath);
        QFile::remove(outputPath);
        return !outImage.isNull();
    }

    return false;
}

bool FileConverter::convertPdf(const QString &filePath, QList<QImage> &outImages)
{
    QString outputDir = QDir::tempPath() + "/phototrick_pdf_" +
                        QFileInfo(filePath).baseName();

    QDir().mkpath(outputDir);

    if (runPythonConverter("pdf_to_images", filePath, outputDir)) {
        QDir dir(outputDir);
        QStringList filters;
        filters << "*.jpg" << "*.png";
        QStringList files = dir.entryList(filters, QDir::Files, QDir::Name);

        for (const QString &file : files) {
            QImage img;
            if (img.load(dir.filePath(file))) {
                outImages.append(img);
            }
        }

        // Cleanup
        for (const QString &file : files) {
            QFile::remove(dir.filePath(file));
        }
        QDir().rmdir(outputDir);
    }

    return !outImages.isEmpty();
}

bool FileConverter::convertOfd(const QString &filePath, QList<QImage> &outImages)
{
    QString outputDir = QDir::tempPath() + "/phototrick_ofd_" +
                        QFileInfo(filePath).baseName();

    QDir().mkpath(outputDir);

    if (runPythonConverter("ofd_to_images", filePath, outputDir)) {
        QDir dir(outputDir);
        QStringList filters;
        filters << "*.jpg" << "*.png";
        QStringList files = dir.entryList(filters, QDir::Files, QDir::Name);

        for (const QString &file : files) {
            QImage img;
            if (img.load(dir.filePath(file))) {
                outImages.append(img);
            }
        }

        // Cleanup
        for (const QString &file : files) {
            QFile::remove(dir.filePath(file));
        }
        QDir().rmdir(outputDir);
    }

    return !outImages.isEmpty();
}

bool FileConverter::extractImagesFromDocx(const QString &filePath, QList<QImage> &outImages)
{
    QString outputDir = QDir::tempPath() + "/phototrick_docx_" +
                        QFileInfo(filePath).baseName();

    QDir().mkpath(outputDir);

    if (runPythonConverter("extract_docx_images", filePath, outputDir)) {
        QDir dir(outputDir);
        QStringList filters;
        filters << "*.jpg" << "*.png" << "*.jpeg" << "*.gif" << "*.bmp";
        QStringList files = dir.entryList(filters, QDir::Files);

        for (const QString &file : files) {
            QImage img;
            if (img.load(dir.filePath(file))) {
                outImages.append(img);
            }
            QFile::remove(dir.filePath(file));
        }

        QDir().rmdir(outputDir);
    }

    return !outImages.isEmpty();
}

bool FileConverter::extractImagesFromXlsx(const QString &filePath, QList<QImage> &outImages)
{
    QString outputDir = QDir::tempPath() + "/phototrick_xlsx_" +
                        QFileInfo(filePath).baseName();

    QDir().mkpath(outputDir);

    if (runPythonConverter("extract_xlsx_images", filePath, outputDir)) {
        QDir dir(outputDir);
        QStringList filters;
        filters << "*.jpg" << "*.png" << "*.jpeg" << "*.gif" << "*.bmp";
        QStringList files = dir.entryList(filters, QDir::Files);

        for (const QString &file : files) {
            QImage img;
            if (img.load(dir.filePath(file))) {
                outImages.append(img);
            }
            QFile::remove(dir.filePath(file));
        }

        QDir().rmdir(outputDir);
    }

    return !outImages.isEmpty();
}

void FileConverter::setPythonPath(const QString &path)
{
    m_pythonPath = path;
}

void FileConverter::setConverterScriptPath(const QString &path)
{
    m_scriptPath = path;
}

bool FileConverter::runPythonConverter(const QString &action, const QString &filePath, const QString &outputPath)
{
    QProcess process;
    QStringList args;

    args << m_scriptPath
         << "--action" << action
         << "--input" << filePath
         << "--output" << outputPath;

    process.start(m_pythonPath, args);

    if (!process.waitForStarted()) {
        qWarning() << "Failed to start Python converter:" << process.errorString();
        return false;
    }

    if (!process.waitForFinished(60000)) { // 60 second timeout
        qWarning() << "Python converter timeout";
        process.kill();
        return false;
    }

    if (process.exitCode() != 0) {
        qWarning() << "Python converter error:" << process.readAllStandardError();
        return false;
    }

    return true;
}
