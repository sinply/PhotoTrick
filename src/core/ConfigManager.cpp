#include "ConfigManager.h"
#include <QDir>

ConfigManager* ConfigManager::instance()
{
    static ConfigManager instance;
    return &instance;
}

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
    , m_settings(new QSettings("PhotoTrick", "PhotoTrick", this))
{
    load();
}

QString ConfigManager::language() const
{
    return m_settings->value("language", "zh_CN").toString();
}

void ConfigManager::setLanguage(const QString &lang)
{
    m_settings->setValue("language", lang);
}

QString ConfigManager::theme() const
{
    return m_settings->value("theme", "light").toString();
}

void ConfigManager::setTheme(const QString &theme)
{
    m_settings->setValue("theme", theme);
}

QString ConfigManager::outputDirectory() const
{
    return m_settings->value("outputDir", QDir::homePath()).toString();
}

void ConfigManager::setOutputDirectory(const QString &dir)
{
    m_settings->setValue("outputDir", dir);
}

QString ConfigManager::defaultOcrBackend() const
{
    return m_settings->value("ocrBackend", "paddle_local").toString();
}

void ConfigManager::setDefaultOcrBackend(const QString &backend)
{
    m_settings->setValue("ocrBackend", backend);
}

QString ConfigManager::apiKey() const
{
    return m_settings->value("apiKey").toString();
}

void ConfigManager::setApiKey(const QString &key)
{
    m_settings->setValue("apiKey", key);
}

QString ConfigManager::baseUrl() const
{
    return m_settings->value("baseUrl").toString();
}

void ConfigManager::setBaseUrl(const QString &url)
{
    m_settings->setValue("baseUrl", url);
}

QString ConfigManager::model() const
{
    return m_settings->value("model").toString();
}

void ConfigManager::setModel(const QString &model)
{
    m_settings->setValue("model", model);
}

// Per-backend API settings
QString ConfigManager::apiKey(const QString &backend) const
{
    return m_settings->value(QString("apiKeys/%1").arg(backend)).toString();
}

void ConfigManager::setApiKey(const QString &backend, const QString &key)
{
    m_settings->setValue(QString("apiKeys/%1").arg(backend), key);
}

QString ConfigManager::baseUrl(const QString &backend) const
{
    return m_settings->value(QString("baseUrls/%1").arg(backend)).toString();
}

void ConfigManager::setBaseUrl(const QString &backend, const QString &url)
{
    m_settings->setValue(QString("baseUrls/%1").arg(backend), url);
}

QString ConfigManager::model(const QString &backend) const
{
    return m_settings->value(QString("models/%1").arg(backend)).toString();
}

void ConfigManager::setModel(const QString &backend, const QString &model)
{
    m_settings->setValue(QString("models/%1").arg(backend), model);
}

QString ConfigManager::defaultExportFormat() const
{
    return m_settings->value("exportFormat", "markdown").toString();
}

void ConfigManager::setDefaultExportFormat(const QString &format)
{
    m_settings->setValue("exportFormat", format);
}

QString ConfigManager::pythonPath() const
{
#ifdef Q_OS_WIN
    // On Windows, use WSL by default (wsl python3)
    return m_settings->value("pythonPath", "wsl").toString();
#else
    return m_settings->value("pythonPath", "python3").toString();
#endif
}

void ConfigManager::setPythonPath(const QString &path)
{
    m_settings->setValue("pythonPath", path);
}

QString ConfigManager::ocrServerPath() const
{
    return m_settings->value("ocrServerPath", "scripts/paddle_server.py").toString();
}

void ConfigManager::setOcrServerPath(const QString &path)
{
    m_settings->setValue("ocrServerPath", path);
}

bool ConfigManager::autoStartOcrServer() const
{
    return m_settings->value("autoStartOcrServer", true).toBool();
}

void ConfigManager::setAutoStartOcrServer(bool autoStart)
{
    m_settings->setValue("autoStartOcrServer", autoStart);
}

bool ConfigManager::stopOcrServerOnExit() const
{
    return m_settings->value("stopOcrServerOnExit", true).toBool();
}

void ConfigManager::setStopOcrServerOnExit(bool stop)
{
    m_settings->setValue("stopOcrServerOnExit", stop);
}

QStringList ConfigManager::recentFiles() const
{
    return m_settings->value("recentFiles").toStringList();
}

void ConfigManager::addRecentFile(const QString &filePath)
{
    QStringList files = recentFiles();
    // Remove if already exists
    files.removeAll(filePath);
    // Add to front
    files.prepend(filePath);
    // Keep only last 20 files
    while (files.size() > 20) {
        files.removeLast();
    }
    m_settings->setValue("recentFiles", files);
}

void ConfigManager::clearRecentFiles()
{
    m_settings->remove("recentFiles");
}

void ConfigManager::load()
{
    m_settings->sync();
}

void ConfigManager::save()
{
    m_settings->sync();
}

QVariant ConfigManager::value(const QString &key, const QVariant &defaultValue) const
{
    return m_settings->value(key, defaultValue);
}

void ConfigManager::setValue(const QString &key, const QVariant &value)
{
    m_settings->setValue(key, value);
}
