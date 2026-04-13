#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QSettings>
#include <QJsonObject>

class ConfigManager : public QObject
{
    Q_OBJECT

public:
    static ConfigManager* instance();

    // General settings
    QString language() const;
    void setLanguage(const QString &lang);

    QString theme() const;
    void setTheme(const QString &theme);

    QString outputDirectory() const;
    void setOutputDirectory(const QString &dir);

    // OCR settings
    QString defaultOcrBackend() const;
    void setDefaultOcrBackend(const QString &backend);

    // API settings
    QString apiKey() const;
    void setApiKey(const QString &key);

    QString baseUrl() const;
    void setBaseUrl(const QString &url);

    QString model() const;
    void setModel(const QString &model);

    // Export settings
    QString defaultExportFormat() const;
    void setDefaultExportFormat(const QString &format);

    // OCR Server settings
    QString pythonPath() const;
    void setPythonPath(const QString &path);

    QString ocrServerPath() const;
    void setOcrServerPath(const QString &path);

    bool autoStartOcrServer() const;
    void setAutoStartOcrServer(bool autoStart);

    bool stopOcrServerOnExit() const;
    void setStopOcrServerOnExit(bool stop);

    // File history
    QStringList recentFiles() const;
    void addRecentFile(const QString &filePath);
    void clearRecentFiles();

    // Load/Save
    void load();
    void save();

private:
    explicit ConfigManager(QObject *parent = nullptr);

    QSettings *m_settings;
};

#endif // CONFIGMANAGER_H
