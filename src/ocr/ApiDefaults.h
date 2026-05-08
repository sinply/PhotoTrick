#ifndef APIDEFAULTS_H
#define APIDEFAULTS_H

#include <QString>
#include <QStringList>
#include <QVector>

struct ApiProviderConfig {
    QString key;
    QString name;
    QString baseUrl;
    QStringList models;
};

namespace ApiDefaults {

QVector<ApiProviderConfig> providersForBackend(const QString &backend);
QString defaultBaseUrlForBackend(const QString &backend);
QString defaultModelForBackend(const QString &backend);
QString buildEndpoint(QString baseUrl, const QString &endpoint);
QString buildMessagesUrl(QString baseUrl);
QString buildChatCompletionsUrl(QString baseUrl);

}

#endif // APIDEFAULTS_H
