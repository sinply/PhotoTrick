#include "FileConverter.h"
#include <QFileInfo>
#include <QDir>
#include <QTemporaryFile>
#include <QDebug>
#include <QCoreApplication>

// Helper function to convert Windows path to WSL path
static QString convertToWslPath(const QString &windowsPath)
{
    QString path = windowsPath;
    // Convert D:/path or D:\path to /mnt/d/path
    if (path.length() >= 2 && path[1] == ':') {
        QString driveLetter = path.left(1).toLower();
        QString pathPart = path.mid(2).replace('\\', '/');
        path = QString("/mnt/%1%2").arg(driveLetter, pathPart);
    }
    return path;
}

FileConverter::FileConverter(QObject *parent)
    : QObject(parent)
    , m_pythonPath("python")
    , m_scriptPath("scripts/file_converter_cli.py")
{
    // If script path is relative, make it absolute based on application directory
    QFileInfo scriptInfo(m_scriptPath);
    if (scriptInfo.isRelative()) {
        QString appDir = QCoreApplication::applicationDirPath();
        m_scriptPath = appDir + "/" + m_scriptPath;
    }
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

    QString pythonCmd = m_pythonPath;
    QString scriptPath = m_scriptPath;
    QString inputPath = filePath;
    QString outPath = outputPath;

#ifdef Q_OS_WIN
    // On Windows: check if using WSL
    if (m_pythonPath == "python" || m_pythonPath.isEmpty() || m_pythonPath == "wsl" || m_pythonPath.endsWith("wsl.exe")) {
        // Convert Windows paths to WSL paths
        inputPath = convertToWslPath(filePath);
        outPath = convertToWslPath(outputPath);
        scriptPath = convertToWslPath(m_scriptPath);

        args << "python3" << scriptPath
             << "--action" << action
             << "--input" << inputPath
             << "--output" << outPath;
        pythonCmd = "wsl.exe";
    } else if (m_pythonPath == "python3") {
        // python3 on Windows typically means WSL
        inputPath = convertToWslPath(filePath);
        outPath = convertToWslPath(outputPath);
        scriptPath = convertToWslPath(m_scriptPath);

        args << "python3" << scriptPath
             << "--action" << action
             << "--input" << inputPath
             << "--output" << outPath;
        pythonCmd = "wsl.exe";
    } else {
        // Native Python on Windows
        args << scriptPath
             << "--action" << action
             << "--input" << inputPath
             << "--output" << outPath;
    }
#else
    // Linux/Mac: use python3 directly
    args << scriptPath
         << "--action" << action
         << "--input" << inputPath
         << "--output" << outPath;
    if (pythonCmd.isEmpty()) {
        pythonCmd = "python3";
    }
#endif

    qDebug() << "FileConverter: Running:" << pythonCmd << args.join(" ");
    process.start(pythonCmd, args);

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
