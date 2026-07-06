#include "OcrManager.h"
#include "ConfigManager.h"
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
        if (!m_baseUrl.isEmpty()) {
            m_currentClient->setBaseUrl(m_baseUrl);
        }
        m_currentClient->setModel(m_model);

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

    if (images.isEmpty()) {
        emit recognitionError(tr("没有可识别的图片"));
        return;
    }

    // 单图直接走 recognizeImage，避免队列开销
    if (images.size() == 1) {
        recognizeImage(images.first(), prompt);
        return;
    }

    // 多图：建立串行队列，连接一次性信号，等前一张完成再发下一张
    m_pendingImages = images;
    m_pendingPrompt = prompt;
    m_pendingTotal = images.size();
    m_pendingIndex = 0;
    m_batchActive = true;

    emit recognitionStarted();

    // 串行钩子：每张完成/失败都推进下一张
    connect(m_currentClient, &OcrInterface::recognitionFinished,
            this, &OcrManager::onBatchFinished, Qt::UniqueConnection);
    connect(m_currentClient, &OcrInterface::recognitionError,
            this, &OcrManager::onBatchError, Qt::UniqueConnection);

    dispatchNextPending();
}

void OcrManager::dispatchNextPending()
{
    if (!m_batchActive || m_pendingIndex >= m_pendingTotal) {
        return;
    }
    emit progress(static_cast<int>(100.0 * m_pendingIndex / m_pendingTotal));
    const QImage &img = m_pendingImages[m_pendingIndex];
    m_currentClient->recognize(img, m_pendingPrompt);
}

void OcrManager::onBatchFinished(const QJsonObject &result)
{
    if (!m_batchActive) return;
    ++m_pendingIndex;
    emit recognitionFinished(result);
    if (m_pendingIndex < m_pendingTotal) {
        dispatchNextPending();
    } else {
        emit progress(100);
        m_batchActive = false;
    }
}

void OcrManager::onBatchError(const QString &error)
{
    if (!m_batchActive) return;
    ++m_pendingIndex;
    emit recognitionError(error);
    if (m_pendingIndex < m_pendingTotal) {
        dispatchNextPending();
    } else {
        m_batchActive = false;
    }
}

void OcrManager::cancelCurrent()
{
    if (m_currentClient) {
        m_currentClient->cancel();
    }
}

PaddleOcr* OcrManager::paddleOcrClient() const
{
    if (m_backend == PaddleOCR_Local) {
        return qobject_cast<PaddleOcr*>(m_currentClient);
    }
    return nullptr;
}
