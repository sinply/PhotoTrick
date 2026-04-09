#include "OcrManager.h"
#include "ocr/OcrInterface.h"
#include "ocr/PaddleOcr.h"
#include "ocr/ClaudeClient.h"
#include "ocr/OpenAIClient.h"

OcrManager::OcrManager(QObject *parent)
    : QObject(parent)
    , m_backend(PaddleOCR_Local)
    , m_currentClient(nullptr)
{
}

void OcrManager::setBackend(Backend backend)
{
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
        m_currentClient->setBaseUrl(m_baseUrl);
        m_currentClient->setModel(m_model);

        connect(m_currentClient, &OcrInterface::recognitionFinished,
                this, &OcrManager::recognitionFinished);
        connect(m_currentClient, &OcrInterface::recognitionError,
                this, &OcrManager::recognitionError);
        connect(m_currentClient, &OcrInterface::progress,
                this, &OcrManager::progress);
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
        m_currentClient->setBaseUrl(url);
    }
}

void OcrManager::setModel(const QString &model)
{
    m_model = model;
    if (m_currentClient) {
        m_currentClient->setModel(model);
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
