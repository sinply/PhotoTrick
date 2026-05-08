#include "ApiDefaults.h"

#include <QRegularExpression>

namespace {

const QVector<ApiProviderConfig> OPENAI_PROVIDERS = {
    {"deepseek", "DeepSeek", "https://api.deepseek.com", {"deepseek-chat"}},
    {"glm", "GLM (智谱)", "https://open.bigmodel.cn/api/paas/v4", {"glm-4v"}},
    {"custom", "自定义", "", {}}
};

const QVector<ApiProviderConfig> CLAUDE_PROVIDERS = {
    {"deepseek", "DeepSeek (Claude兼容)", "https://api.deepseek.com/anthropic", {"deepseek-chat", "deepseek-reasoner"}},
    {"anthropic", "Anthropic Claude", "https://api.anthropic.com", {"claude-3-5-sonnet-20241022"}},
    {"custom", "自定义", "", {}}
};

QString firstProviderBaseUrl(const QVector<ApiProviderConfig> &providers)
{
    for (const ApiProviderConfig &provider : providers) {
        if (!provider.baseUrl.isEmpty()) {
            return provider.baseUrl;
        }
    }
    return QString();
}

QString firstProviderModel(const QVector<ApiProviderConfig> &providers)
{
    for (const ApiProviderConfig &provider : providers) {
        if (!provider.models.isEmpty()) {
            return provider.models.first();
        }
    }
    return QString();
}

} // namespace

namespace ApiDefaults {

QVector<ApiProviderConfig> providersForBackend(const QString &backend)
{
    if (backend == QStringLiteral("claude_format")) {
        return CLAUDE_PROVIDERS;
    }
    if (backend == QStringLiteral("openai_format")) {
        return OPENAI_PROVIDERS;
    }
    return {};
}

QString defaultBaseUrlForBackend(const QString &backend)
{
    return firstProviderBaseUrl(providersForBackend(backend));
}

QString defaultModelForBackend(const QString &backend)
{
    return firstProviderModel(providersForBackend(backend));
}

QString buildEndpoint(QString baseUrl, const QString &endpoint)
{
    baseUrl = baseUrl.trimmed();
    while (baseUrl.endsWith('/')) {
        baseUrl.chop(1);
    }

    const QString normalizedEndpoint = endpoint.startsWith('/')
        ? endpoint
        : QStringLiteral("/") + endpoint;
    if (baseUrl.endsWith(normalizedEndpoint)) {
        return baseUrl;
    }

    const QRegularExpression versionSuffix(QStringLiteral(R"(/v\d+$)"));
    if (versionSuffix.match(baseUrl).hasMatch()) {
        return baseUrl + normalizedEndpoint;
    }

    return baseUrl + QStringLiteral("/v1") + normalizedEndpoint;
}

QString buildMessagesUrl(QString baseUrl)
{
    return buildEndpoint(baseUrl, QStringLiteral("/messages"));
}

QString buildChatCompletionsUrl(QString baseUrl)
{
    return buildEndpoint(baseUrl, QStringLiteral("/chat/completions"));
}

} // namespace ApiDefaults
