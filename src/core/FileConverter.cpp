#include "FileConverter.h"
#include <QFileInfo>
#include <QDir>
#include <QTemporaryFile>
#include <QDebug>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QImageReader>

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

// Build command line args for the python converter, handling WSL/native python.
// Returns the python command to execute; fills `args`.
static QString buildConverterArgs(const QString &pythonPath, const QString &scriptPath,
                                   const QString &action, const QString &inputPath,
                                   const QString &outputPath, QStringList &args)
{
    QString pythonCmd = pythonPath;
    QString script = scriptPath;
    QString input = inputPath;
    QString output = outputPath;

#ifdef Q_OS_WIN
    if (pythonPath == "python" || pythonPath.isEmpty() || pythonPath == "wsl" || pythonPath.endsWith("wsl.exe")) {
        input = convertToWslPath(inputPath);
        output = convertToWslPath(outputPath);
        script = convertToWslPath(scriptPath);
        args << "python3" << script << "--action" << action << "--input" << input << "--output" << output;
        pythonCmd = "wsl.exe";
    } else if (pythonPath == "python3") {
        input = convertToWslPath(inputPath);
        output = convertToWslPath(outputPath);
        script = convertToWslPath(scriptPath);
        args << "python3" << script << "--action" << action << "--input" << input << "--output" << output;
        pythonCmd = "wsl.exe";
    } else {
        args << script << "--action" << action << "--input" << input << "--output" << output;
    }
#else
    args << script << "--action" << action << "--input" << input << "--output" << output;
    if (pythonCmd.isEmpty()) {
        pythonCmd = "python3";
    }
#endif
    return pythonCmd;
}

// 生成包含 pid 的唯一临时目录前缀，避免同名文件互踩
static QString uniqueTempPrefix(const QString &filePath)
{
    return QFileInfo(filePath).completeBaseName() + "_" +
           QString::number(QCoreApplication::applicationPid());
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
    QString outputPath = QDir::tempPath() + "/phototrick_heic_" +
                         uniqueTempPrefix(filePath) + ".jpg";

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
                        uniqueTempPrefix(filePath);

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
                        uniqueTempPrefix(filePath);

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
                        uniqueTempPrefix(filePath);

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
    QString pythonCmd = buildConverterArgs(m_pythonPath, m_scriptPath,
                                           action, filePath, outputPath, args);

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

// ==================== 异步接口 ====================

bool FileConverter::requestConvert(const QString &filePath)
{
    if (m_busy) {
        emit conversionError(filePath, tr("已有转换任务在执行"));
        return false;
    }

    QString ext = QFileInfo(filePath).suffix().toLower();
    QString action;
    QString prefix;

    if (ext == "pdf") {
        action = "pdf_to_images"; prefix = "pdf";
    } else if (ext == "docx") {
        action = "extract_docx_images"; prefix = "docx";
    } else if (ext == "xlsx") {
        action = "extract_xlsx_images"; prefix = "xlsx";
    } else if (ext == "heic") {
        // heic 的 python 接口期望一个 .jpg 文件路径，但我们统一用目录容器
        // 让输出落到 outputDir/output.jpg，再从目录扫描收集
        action = "heic_to_jpg"; prefix = "heic";
    } else {
        emit conversionError(filePath, tr("不支持的文件格式: %1").arg(ext.toUpper()));
        return false;
    }

    // 用 hash + pid 生成唯一目录，避免同名文件互踩
    QString key = QString::fromLatin1(QCryptographicHash::hash(
        (filePath + QString::number(QCoreApplication::applicationPid())).toUtf8(),
        QCryptographicHash::Sha1).toHex()).left(8);
    QString outputDir = QDir::tempPath() + QString("/phototrick_%1_%2").arg(prefix, key);

    startAsyncConvert(filePath, action, outputDir, /*outputIsFile=*/false);
    return true;
}

void FileConverter::startAsyncConvert(const QString &filePath, const QString &action,
                                      const QString &outputPath, bool outputIsFile)
{
    // 确保输出目录存在
    QFileInfo outInfo(outputPath);
    QString outputDir = outputIsFile ? outInfo.absolutePath() : outputPath;
    QDir().mkpath(outputDir);

    QStringList args;
    QString pythonCmd = buildConverterArgs(m_pythonPath, m_scriptPath,
                                            action, filePath, outputPath, args);

    m_currentFilePath = filePath;
    m_currentOutputPath = outputPath;
    m_currentOutputIsFile = outputIsFile;
    m_busy = true;

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus status) {
                Q_UNUSED(status);
                if (exitCode != 0) {
                    QString err = QString::fromUtf8(m_process->readAllStandardOutput());
                    finishAsync(false, tr("格式转换失败（退出码 %1）: %2")
                                    .arg(exitCode).arg(err.trimmed()));
                } else {
                    finishAsync(true, QString());
                }
            });
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        finishAsync(false, tr("无法启动转换进程: %1").arg(m_process->errorString()));
    });

    qDebug() << "FileConverter: async running:" << pythonCmd << args.join(" ");
    m_process->start(pythonCmd, args);

    if (!m_process->waitForStarted(3000)) {
        QString err = m_process->errorString();
        m_busy = false;
        m_process->deleteLater();
        m_process = nullptr;
        emit conversionError(filePath, tr("无法启动转换进程: %1").arg(err));
    }
}

void FileConverter::collectImagesFromDir(const QString &dir, QList<QImage> &out)
{
    QDir d(dir);
    QStringList filters;
    filters << "*.jpg" << "*.png" << "*.jpeg" << "*.bmp" << "*.gif";
    QStringList files = d.entryList(filters, QDir::Files, QDir::Name);

    for (const QString &file : files) {
        QImageReader reader(d.filePath(file));
        reader.setAutoTransform(true);  // 按 EXIF 方向自动旋转
        QImage img = reader.read();
        if (!img.isNull()) {
            out.append(img);
        }
    }
}

void FileConverter::cleanupPath(const QString &path, bool isFile)
{
    if (isFile) {
        QFile::remove(path);
        return;
    }
    QDir dir(path);
    if (!dir.exists()) return;
    QStringList filters;
    filters << "*.jpg" << "*.png" << "*.jpeg" << "*.bmp" << "*.gif";
    for (const QString &file : dir.entryList(filters, QDir::Files)) {
        QFile::remove(dir.filePath(file));
    }
    dir.rmdir(path);
}

void FileConverter::finishAsync(bool success, const QString &errorMessage)
{
    QString filePath = m_currentFilePath;
    QString outputPath = m_currentOutputPath;
    bool isFile = m_currentOutputIsFile;
    m_busy = false;

    if (!success) {
        cleanupPath(outputPath, isFile);
        if (m_process) {
            m_process->deleteLater();
        }
        emit conversionError(filePath, errorMessage);
        return;
    }

    QList<QImage> images;
    if (isFile) {
        QImage img;
        if (img.load(outputPath)) {
            images.append(img);
        }
    } else {
        collectImagesFromDir(outputPath, images);
    }
    cleanupPath(outputPath, isFile);

    if (m_process) {
        m_process->deleteLater();
    }

    if (images.isEmpty()) {
        emit conversionError(filePath, tr("转换未产生图片，请检查文件是否损坏或转换为PNG/JPG后重试"));
    } else {
        emit conversionFinished(filePath, images);
    }
}

void FileConverter::cancel()
{
    if (!m_busy || !m_process) {
        m_busy = false;
        return;
    }
    qDebug() << "FileConverter: cancelling current conversion";
    m_process->kill();
    // finished/errorOccurred 会触发 finishAsync，但被 kill 时 exitCode 非 0 会走错误路径；
    // 这里直接清理避免信号污染主流程（主流程取消后不再监听）
    disconnect(m_process, nullptr, this, nullptr);
    QString filePath = m_currentFilePath;
    QString outputPath = m_currentOutputPath;
    bool isFile = m_currentOutputIsFile;
    m_busy = false;
    cleanupPath(outputPath, isFile);
    m_process->deleteLater();
    m_process = nullptr;
    // 不发 conversionError，避免取消后弹错误日志干扰
    Q_UNUSED(filePath);
}
