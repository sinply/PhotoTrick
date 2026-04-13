#ifndef OCRSERVERMANAGER_H
#define OCRSERVERMANAGER_H

#include <QObject>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QTimer>

class OcrServerManager : public QObject
{
    Q_OBJECT

public:
    enum ServerStatus {
        NotRunning,
        Starting,
        Running,
        Error
    };

    explicit OcrServerManager(QObject *parent = nullptr);
    ~OcrServerManager();

    void setServerPath(const QString &pythonPath, const QString &scriptPath);
    void setServerUrl(const QString &url);

    ServerStatus status() const;
    bool isRunning() const;

    void start();
    void stop();
    void checkStatus();

signals:
    void statusChanged(ServerStatus status);
    void serverError(const QString &message);

private slots:
    void onProcessStarted();
    void onProcessError(QProcess::ProcessError error);
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onHealthCheckTimeout();

private:
    void setStatus(ServerStatus status);
    void performHealthCheck();
    void actuallyStartServer();  // 实际启动服务器的内部方法

    QProcess *m_process;
    QNetworkAccessManager *m_networkManager;
    QString m_pythonPath;
    QString m_scriptPath;
    QString m_serverUrl;
    ServerStatus m_status;
    QTimer *m_healthCheckTimer;
    int m_healthCheckRetries;
    static const int MAX_HEALTH_RETRIES = 10;
};

#endif // OCRSERVERMANAGER_H
