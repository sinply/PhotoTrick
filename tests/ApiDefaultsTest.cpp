#include "../src/ocr/ApiDefaults.h"

#include <QDebug>

namespace {

int failures = 0;

void checkEqual(const QString &name, const QString &actual, const QString &expected)
{
    if (actual != expected) {
        qCritical().noquote() << name << "expected:" << expected << "actual:" << actual;
        ++failures;
    }
}

} // namespace

int main()
{
    checkEqual(
        "claude default baseUrl",
        ApiDefaults::defaultBaseUrlForBackend("claude_format"),
        "https://api.deepseek.com/anthropic");
    checkEqual(
        "claude default model",
        ApiDefaults::defaultModelForBackend("claude_format"),
        "deepseek-chat");
    checkEqual(
        "claude messages endpoint",
        ApiDefaults::buildMessagesUrl("https://api.deepseek.com/anthropic"),
        "https://api.deepseek.com/anthropic/v1/messages");
    checkEqual(
        "claude messages endpoint idempotent",
        ApiDefaults::buildMessagesUrl("https://api.deepseek.com/anthropic/v1/messages"),
        "https://api.deepseek.com/anthropic/v1/messages");
    checkEqual(
        "openai chat endpoint",
        ApiDefaults::buildChatCompletionsUrl("https://open.bigmodel.cn/api/paas/v4"),
        "https://open.bigmodel.cn/api/paas/v4/chat/completions");

    return failures == 0 ? 0 : 1;
}
