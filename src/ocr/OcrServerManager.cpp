#include "OcrServerManager.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QDateTime>

// Helper function to log to file
static void logToFile(const QString &message)
{
    QString logPath = QCoreApplication::applicationDirPath() + "/ocr_server_debug.log";
    QFile file(logPath);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        file.write(QString("[%1] %2\n").arg(timestamp, message).toUtf8());
        file.close();
    }
}

OcrServerManager::OcrServerManager(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_networkManager(new QNetworkAccessManager(this))
    , m_pythonPath("python3")
    , m_scriptPath("scripts/paddle_server.py")
    , m_serverUrl("http://127.0.0.1:5000")
    , m_status(NotRunning)
    , m_healthCheckTimer(new QTimer(this))
    , m_healthCheckRetries(0)
{
    connect(m_process, &QProcess::started,
            this, &OcrServerManager::onProcessStarted);
    connect(m_process, &QProcess::errorOccurred,
            this, &OcrServerManager::onProcessError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &OcrServerManager::onProcessFinished);
    connect(m_healthCheckTimer, &QTimer::timeout,
            this, &OcrServerManager::onHealthCheckTimeout);
}

OcrServerManager::~OcrServerManager()
{
    stop();
}

void OcrServerManager::setServerPath(const QString &pythonPath, const QString &scriptPath)
{
    m_pythonPath = pythonPath;
    m_scriptPath = scriptPath;
}

void OcrServerManager::setServerUrl(const QString &url)
{
    logToFile(QString("setServerUrl called with: %1").arg(url));
    m_serverUrl = url;
}

OcrServerManager::ServerStatus OcrServerManager::status() const
{
    return m_status;
}

bool OcrServerManager::isRunning() const
{
    return m_status == Running;
}

void OcrServerManager::start()
{
    logToFile(QString("start() called, current state: %1, serverUrl: %2, status: %3").arg(m_process->state()).arg(m_serverUrl).arg(m_status));

    // 首先检查服务器是否已经在运行（健康检查）
    if (m_status == Running) {
        logToFile("Server already marked as Running, returning");
        return;
    }

    // 如果已经在启动中，不要重复启动
    if (m_status == Starting) {
        logToFile("Server already Starting, returning");
        return;
    }

    if (m_process->state() != QProcess::NotRunning) {
        qDebug() << "OcrServerManager: Process already running or starting";
        logToFile("Process already running or starting, returning");
        return;  // Already running or starting
    }

    // 先做一次健康检查，看服务器是否已经在运行
    logToFile("Performing initial health check before starting...");
    QNetworkRequest request(QUrl(m_serverUrl + "/health"));
    QNetworkReply *reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            logToFile(QString("Initial health check response: %1").arg(QString::fromUtf8(data)));
            if (data.contains("\"status\":\"ok\"")) {
                logToFile("Server already running! Setting status to Running");
                setStatus(Running);
                return;
            }
        }

        logToFile(QString("Initial health check failed or server not running: %1").arg(reply->errorString()));
        // 服务器没有运行，启动它
        actuallyStartServer();
    });
}

void OcrServerManager::actuallyStartServer()
{
    logToFile("actuallyStartServer() called");

    setStatus(Starting);
    m_healthCheckRetries = 0;

    // Build command arguments
    QString scriptPath = m_scriptPath;

    // If script path is relative, make it absolute based on application directory
    QFileInfo scriptInfo(scriptPath);
    if (scriptInfo.isRelative()) {
        QString appDir = QCoreApplication::applicationDirPath();
        scriptPath = appDir + "/" + scriptPath;
    }

    QStringList args;
    QString pythonCmd;

#ifdef Q_OS_WIN
    // On Windows with WSL: use wsl.exe to run python3
    // Use full path to wsl.exe for reliability
    if (m_pythonPath == "python" || m_pythonPath.isEmpty() || m_pythonPath == "wsl") {
        // Use WSL by default - convert Windows path to WSL path
        QString wslPath = scriptPath;
        // Convert D:/path or D:\path to /mnt/d/path
        if (wslPath.length() >= 2 && wslPath[1] == ':') {
            QString driveLetter = wslPath.left(1).toLower();
            QString pathPart = wslPath.mid(2).replace('\\', '/');
            wslPath = QString("/mnt/%1%2").arg(driveLetter, pathPart);
        }
        args << "python3" << wslPath;
        pythonCmd = "wsl.exe";
    } else if (m_pythonPath.endsWith("wsl") || m_pythonPath.endsWith("wsl.exe")) {
        // WSL path specified - convert Windows path to WSL path
        QString wslPath = scriptPath;
        if (wslPath.length() >= 2 && wslPath[1] == ':') {
            QString driveLetter = wslPath.left(1).toLower();
            QString pathPart = wslPath.mid(2).replace('\\', '/');
            wslPath = QString("/mnt/%1%2").arg(driveLetter, pathPart);
        }
        args << "python3" << wslPath;
        pythonCmd = "wsl.exe";
    } else {
        // Native Python path specified
        args << scriptPath;
        pythonCmd = m_pythonPath;
    }
#else
    // Linux/Mac: use python3
    args << scriptPath;
    pythonCmd = m_pythonPath.isEmpty() ? "python3" : m_pythonPath;
#endif

    QString logMsg = QString("Starting server with: %1 %2").arg(pythonCmd, args.join(" "));
    qDebug() << "OcrServerManager:" << logMsg;
    logToFile(logMsg);
    logToFile(QString("Script path: %1").arg(scriptPath));
    logToFile(QString("Working directory: %1").arg(QCoreApplication::applicationDirPath()));

    m_process->setWorkingDirectory(QCoreApplication::applicationDirPath());
    m_process->start(pythonCmd, args);

    logToFile(QString("Process started, waiting for result..."));
}

void OcrServerManager::stop()
{
    m_healthCheckTimer->stop();

    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(3000)) {
            m_process->kill();
            m_process->waitForFinished(1000);
        }
    }

    setStatus(NotRunning);
}

void OcrServerManager::checkStatus()
{
    if (m_status == Starting) {
        return;  // Already checking
    }
    performHealthCheck();
}

void OcrServerManager::setStatus(ServerStatus status)
{
    if (m_status != status) {
        logToFile(QString("Status changed from %1 to %2").arg(m_status).arg(status));
        m_status = status;
        emit statusChanged(status);
    }
}

void OcrServerManager::onProcessStarted()
{
    logToFile("Process started successfully (onProcessStarted)");
    qDebug() << "OcrServerManager: Process started successfully";
    // Process started, begin health checks
    m_healthCheckTimer->start(2000);  // Check every 2 seconds
}

void OcrServerManager::onProcessError(QProcess::ProcessError error)
{
    QString errorMsg = QString("Process error: %1 - %2").arg(error).arg(m_process->errorString());
    logToFile(errorMsg);
    qDebug() << "OcrServerManager:" << errorMsg;

    // On Windows with WSL: wsl.exe may report "crashed" when it exits after launching server
    // Don't immediately set Error status - let health check determine actual server status
    // Only report error if server is not already running
    if (m_status == Running) {
        logToFile("Process error ignored - server already running (WSL behavior)");
        return;
    }

    m_healthCheckTimer->stop();
    setStatus(Error);
    emit serverError(tr("OCR服务器启动失败: %1").arg(m_process->errorString()));
}

void OcrServerManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QString stdoutOutput = QString::fromUtf8(m_process->readAllStandardOutput());
    QString stderrOutput = QString::fromUtf8(m_process->readAllStandardError());

    logToFile(QString("Process finished with exit code: %1, status: %2").arg(exitCode).arg(exitStatus));
    logToFile(QString("Stdout: %1").arg(stdoutOutput.left(500)));
    logToFile(QString("Stderr: %1").arg(stderrOutput.left(500)));

    qDebug() << "OcrServerManager: Process finished with exit code:" << exitCode << "status:" << exitStatus;
    qDebug() << "OcrServerManager: Process output:" << stdoutOutput;
    qDebug() << "OcrServerManager: Process error output:" << stderrOutput;

    // On Windows with WSL: wsl.exe exits immediately after launching the background process
    // The Python server continues running in WSL background
    // Don't stop health check timer - let it determine if server is still running
    // Only set status to NotRunning if server is not already confirmed running
    if (m_status != Running) {
        m_healthCheckTimer->stop();
        setStatus(NotRunning);
    }
}

void OcrServerManager::onHealthCheckTimeout()
{
    if (m_status == Running) {
        // Periodic health check while running
        performHealthCheck();
    } else if (m_status == Starting) {
        // Still starting, check if ready
        m_healthCheckRetries++;
        if (m_healthCheckRetries > MAX_HEALTH_RETRIES) {
            m_healthCheckTimer->stop();
            setStatus(Error);
            emit serverError(tr("OCR服务器启动超时"));
            return;
        }
        performHealthCheck();
    }
}

void OcrServerManager::performHealthCheck()
{
    QUrl url(m_serverUrl + "/health");
    QNetworkRequest request(url);

    logToFile(QString("Performing health check to %1, current status: %2").arg(url.toString()).arg(m_status));

    QNetworkReply *reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            logToFile(QString("Health check response: %1").arg(QString::fromUtf8(data)));
            if (data.contains("\"status\":\"ok\"")) {
                if (m_status == Starting) {
                    m_healthCheckRetries = 0;
                }
                logToFile("Health check passed, setting status to Running");
                setStatus(Running);
            } else {
                if (m_status == Running) {
                    setStatus(Error);
                    emit serverError(tr("OCR服务器返回异常状态"));
                }
            }
        } else {
            // Connection failed
            logToFile(QString("Health check failed: %1").arg(reply->errorString()));
            if (m_status == Running) {
                setStatus(Error);
                emit serverError(tr("OCR服务器连接丢失"));
            }
            // If starting, keep trying (handled by retry counter)
        }
    });
}
