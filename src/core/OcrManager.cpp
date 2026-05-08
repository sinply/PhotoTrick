#include "OcrManager.h"
#include "ConfigManager.h"
#include "ocr/OcrInterface.h"
#include "ocr/PaddleOcr.h"
#include "ocr/ApiDefaults.h"
#include "ocr/ClaudeClient.h"
#include "ocr/OpenAIClient.h"

namespace {

QString backendKey(OcrManager::Backend backend)
{
    switch (backend) {
    case OcrManager::Claude_Format:
        return QStringLiteral("claude_format");
    case OcrManager::OpenAI_Format:
        return QStringLiteral("openai_format");
    case OcrManager::PaddleOCR_Local:
        return QStringLiteral("paddle_local");
    }
    return QString();
}

} // namespace

OcrManager::OcrManager(QObject *parent)
    : QObject(parent)
    , m_backend(PaddleOCR_Local)
    , m_currentClient(nullptr)
{
}

void OcrManager::setBackend(Backend backend)
{
    // Note: We don't stop the OCR server when switching to online mode
    // The server keeps running so it's immediately available when switching back
    if (m_currentClient) {
        delete m_currentClient;
        m_currentClient = nullptr;
    }

    m_backend = backend;

    switch (backend) {
    case PaddleOCR_Local:
        m_currentClient = new PaddleOcr(this);
        break;
    case Claude_Format:
        m_currentClient = new ClaudeClient(this);
        break;
    case OpenAI_Format:
        m_currentClient = new OpenAIClient(this);
        break;
    }

    if (m_currentClient) {
        m_currentClient->setApiKey(m_apiKey);
        const QString key = backendKey(m_backend);
        const QString baseUrl = m_baseUrl.isEmpty()
            ? ApiDefaults::defaultBaseUrlForBackend(key)
            : m_baseUrl;
        if (!baseUrl.isEmpty()) {
            m_currentClient->setBaseUrl(baseUrl);
        }
        const QString model = m_model.isEmpty()
            ? ApiDefaults::defaultModelForBackend(key)
            : m_model;
        if (!model.isEmpty()) {
            m_currentClient->setModel(model);
        }

        connect(m_currentClient, &OcrInterface::recognitionFinished,
                this, &OcrManager::recognitionFinished);
        connect(m_currentClient, &OcrInterface::recognitionError,
                this, &OcrManager::recognitionError);
        connect(m_currentClient, &OcrInterface::progress,
                this, &OcrManager::progress);
    }

    // Auto-start local OCR server if switching to PaddleOCR_Local
    if (backend == PaddleOCR_Local && ConfigManager::instance()->autoStartOcrServer()) {
        auto *paddleOcr = qobject_cast<PaddleOcr*>(m_currentClient);
        if (paddleOcr) {
            // Connect server status signal
            connect(paddleOcr, &PaddleOcr::serverStatusChanged,
                    this, &OcrManager::serverStatusChanged);

            if (!paddleOcr->isServerRunning()) {
                paddleOcr->startServer();
            }
        }
    }
}

OcrManager::Backend OcrManager::backend() const
{
    return m_backend;
}

void OcrManager::setApiKey(const QString &key)
{
    m_apiKey = key;
    if (m_currentClient) {
        m_currentClient->setApiKey(key);
    }
}

void OcrManager::setBaseUrl(const QString &url)
{
    m_baseUrl = url;
    if (m_currentClient) {
        QString effectiveUrl = url;
        if (effectiveUrl.isEmpty()) {
            effectiveUrl = ApiDefaults::defaultBaseUrlForBackend(backendKey(m_backend));
        }
        m_currentClient->setBaseUrl(effectiveUrl);
    }
}

void OcrManager::setModel(const QString &model)
{
    m_model = model;
    if (m_currentClient) {
        QString effectiveModel = model;
        if (effectiveModel.isEmpty()) {
            effectiveModel = ApiDefaults::defaultModelForBackend(backendKey(m_backend));
        }
        m_currentClient->setModel(effectiveModel);
    }
}

void OcrManager::recognizeImage(const QImage &image, const QString &prompt)
{
    if (!m_currentClient) {
        emit recognitionError(tr("OCR客户端未初始化"));
        return;
    }

    emit recognitionStarted();
    m_currentClient->recognize(image, prompt);
}

void OcrManager::recognizeImages(const QList<QImage> &images, const QString &prompt)
{
    if (!m_currentClient) {
        emit recognitionError(tr("OCR客户端未初始化"));
        return;
    }

    emit recognitionStarted();

    // For multiple images, process sequentially
    // Could be enhanced to process in parallel
    for (int i = 0; i < images.size(); ++i) {
        emit progress(static_cast<int>(100.0 * i / images.size()));
        m_currentClient->recognize(images[i], prompt);
    }

    emit progress(100);
}

PaddleOcr* OcrManager::paddleOcrClient() const
{
    if (m_backend == PaddleOCR_Local) {
        return qobject_cast<PaddleOcr*>(m_currentClient);
    }
    return nullptr;
}
