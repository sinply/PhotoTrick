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

QString ConfigManager::defaultExportFormat() const
{
    return m_settings->value("exportFormat", "markdown").toString();
}

void ConfigManager::setDefaultExportFormat(const QString &format)
{
    m_settings->setValue("exportFormat", format);
}

void ConfigManager::load()
{
    m_settings->sync();
}

void ConfigManager::save()
{
    m_settings->sync();
}
