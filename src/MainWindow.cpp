#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "ui/FileListView.h"
#include "ui/ProcessingPanel.h"
#include "ui/ResultPreview.h"
#include "ui/CategoryView.h"
#include "ui/SettingsDialog.h"
#include "core/OcrManager.h"
#include "core/ConfigManager.h"
#include "core/FileConverter.h"
#include "processors/InvoiceRecognizer.h"
#include "processors/TableExtractor.h"
#include "processors/ItineraryRecognizer.h"
#include "models/InvoiceData.h"
#include "models/TableData.h"
#include "models/ItineraryData.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QDebug>
#include <QImageReader>
#include <QFileInfo>
#include <QTimer>
#include <QJsonDocument>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_mainSplitter(nullptr)
    , m_rightSplitter(nullptr)
    , m_fileListView(nullptr)
    , m_processingPanel(nullptr)
    , m_resultPreview(nullptr)
    , m_categoryView(nullptr)
    , m_statusLabel(nullptr)
    , m_progressBar(nullptr)
    , m_ocrManager(nullptr)
    , m_invoiceRecognizer(nullptr)
    , m_tableExtractor(nullptr)
    , m_itineraryRecognizer(nullptr)
    , m_currentFileIndex(0)
    , m_isProcessing(false)
    , m_fileConverter(nullptr)
{
    ui->setupUi(this);

    // Initialize OCR Manager
    try {
        m_ocrManager = new OcrManager(this);
    } catch (const std::exception &e) {
        QMessageBox::critical(this, tr("错误"), tr("初始化OCR管理器失败: %1").arg(e.what()));
    }

    // Initialize processors
    m_invoiceRecognizer = new InvoiceRecognizer(this);
    m_tableExtractor = new TableExtractor(this);
    m_itineraryRecognizer = new ItineraryRecognizer(this);

    // Set OCR manager for processors
    m_invoiceRecognizer->setOcrManager(m_ocrManager);
    m_tableExtractor->setOcrManager(m_ocrManager);
    m_itineraryRecognizer->setOcrManager(m_ocrManager);

    // Initialize file converter
    m_fileConverter = new FileConverter(this);

    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupCentralWidget();
    setupConnections();

    // Load saved API settings
    loadApiSettings();

    // Initialize backend from settings
    QString backend = ConfigManager::instance()->defaultOcrBackend();
    qDebug() << "MainWindow: Default OCR backend from settings:" << backend;
    onOcrBackendChanged(backend);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupMenuBar()
{
    // File menu
    QMenu *fileMenu = menuBar()->addMenu(tr("文件(&F)"));
    fileMenu->addAction(tr("添加文件(&A)..."), this, &MainWindow::onActionAddFiles, QKeySequence::Open);
    fileMenu->addAction(tr("添加文件夹(&D)..."), this, &MainWindow::onActionAddFolder);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("退出(&X)"), this, &QWidget::close, QKeySequence::Quit);

    // Edit menu
    QMenu *editMenu = menuBar()->addMenu(tr("编辑(&E)"));
    editMenu->addAction(tr("清空列表(&C)"), this, [this]() {
        if (m_fileListView) {
            m_fileListView->clearFiles();
        }
    });

    // Settings menu
    QMenu *settingsMenu = menuBar()->addMenu(tr("设置(&S)"));
    settingsMenu->addAction(tr("首选项(&P)..."), this, &MainWindow::onActionSettings);

    // Help menu
    QMenu *helpMenu = menuBar()->addMenu(tr("帮助(&H)"));
    helpMenu->addAction(tr("关于(&A)"), this, &MainWindow::onActionAbout);
}

void MainWindow::setupToolBar()
{
    QToolBar *toolBar = addToolBar(tr("主工具栏"));
    toolBar->setMovable(false);

    toolBar->addAction(tr("添加文件"), this, &MainWindow::onActionAddFiles);
    toolBar->addAction(tr("添加文件夹"), this, &MainWindow::onActionAddFolder);
    toolBar->addSeparator();
    toolBar->addAction(tr("设置"), this, &MainWindow::onActionSettings);
}

void MainWindow::setupStatusBar()
{
    m_statusLabel = new QLabel(tr("就绪"));
    m_statusLabel->setFrameStyle(QFrame::NoFrame);
    statusBar()->addWidget(m_statusLabel, 1);

    m_progressBar = new QProgressBar();
    m_progressBar->setTextVisible(true);
    m_progressBar->setFormat("%p%");
    m_progressBar->setFixedWidth(150);
    m_progressBar->hide();
    statusBar()->addPermanentWidget(m_progressBar);
}

void MainWindow::setupCentralWidget()
{
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_rightSplitter = new QSplitter(Qt::Vertical, this);

    // Left: File list
    m_fileListView = new FileListView(this);
    m_mainSplitter->addWidget(m_fileListView);

    // Right top: Processing panel
    m_processingPanel = new ProcessingPanel(this);
    m_rightSplitter->addWidget(m_processingPanel);

    // Right bottom: Result preview
    m_resultPreview = new ResultPreview(this);
    m_rightSplitter->addWidget(m_resultPreview);

    m_mainSplitter->addWidget(m_rightSplitter);

    // Set initial sizes
    m_mainSplitter->setSizes({300, 900});
    m_rightSplitter->setSizes({200, 600});

    setCentralWidget(m_mainSplitter);
}

void MainWindow::setupConnections()
{
    connect(m_fileListView, &FileListView::filesAdded, this, [this]() {
        m_statusLabel->setText(tr("已添加 %1 个文件").arg(m_fileListView->fileCount()));
    });

    connect(m_processingPanel, &ProcessingPanel::startProcessing, this, &MainWindow::onProcessingStarted);
    connect(m_processingPanel, &ProcessingPanel::cancelProcessing, this, [this]() {
        m_isProcessing = false;
        m_pendingFiles.clear();
        m_statusLabel->setText(tr("已取消"));
        m_progressBar->hide();
    });

    // Connect backend change signal
    connect(m_processingPanel, &ProcessingPanel::backendChanged, this, &MainWindow::onOcrBackendChanged);

    // Connect server status change
    connect(m_ocrManager, &OcrManager::serverStatusChanged, this, &MainWindow::onServerStatusChanged);

    // Connect OCR manager progress
    connect(m_ocrManager, &OcrManager::progress, this, &MainWindow::onProcessingProgress);
    connect(m_ocrManager, &OcrManager::recognitionError, this, &MainWindow::onProcessingError);

    // Connect processor signals
    connect(m_invoiceRecognizer, &InvoiceRecognizer::recognitionFinished,
            this, &MainWindow::onInvoiceRecognized);
    connect(m_invoiceRecognizer, &InvoiceRecognizer::recognitionError,
            this, &MainWindow::onProcessingError);

    connect(m_tableExtractor, &TableExtractor::extractionFinished,
            this, &MainWindow::onTableExtracted);
    connect(m_tableExtractor, &TableExtractor::extractionError,
            this, &MainWindow::onProcessingError);

    connect(m_itineraryRecognizer, &ItineraryRecognizer::recognitionFinished,
            this, &MainWindow::onItineraryRecognized);
    connect(m_itineraryRecognizer, &ItineraryRecognizer::recognitionError,
            this, &MainWindow::onProcessingError);

    auto logRawOcr = [this](const QString &type, const QJsonObject &result) {
        if (!m_processingPanel) {
            return;
        }
        QString raw = QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact));
        if (raw.isEmpty()) {
            raw = "{}";
        }
        const int maxLen = 2000;
        if (raw.size() > maxLen) {
            raw = raw.left(maxLen) + "...";
        }
        m_processingPanel->appendLog(tr("  OCR原始结果(%1): %2").arg(type, raw));
    };
    connect(m_invoiceRecognizer, &InvoiceRecognizer::rawOcrReceived, this, [logRawOcr](const QJsonObject &result) {
        logRawOcr(QStringLiteral("发票"), result);
    });
    connect(m_tableExtractor, &TableExtractor::rawOcrReceived, this, [logRawOcr](const QJsonObject &result) {
        logRawOcr(QStringLiteral("表格"), result);
    });
    connect(m_itineraryRecognizer, &ItineraryRecognizer::rawOcrReceived, this, [logRawOcr](const QJsonObject &result) {
        logRawOcr(QStringLiteral("行程单"), result);
    });

    // Connect API settings change to save
    connect(m_processingPanel, &ProcessingPanel::apiSettingsChanged, this, [this]() {
        saveApiSettings();
    });
}

void MainWindow::onActionAddFiles()
{
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        tr("选择文件"),
        QString(),
        tr("支持的文件 (*.jpg *.jpeg *.png *.bmp *.gif *.tiff *.webp *.heic *.pdf *.ofd *.docx *.xlsx);;")
        + tr("图片 (*.jpg *.jpeg *.png *.bmp *.gif *.tiff *.webp *.heic);;")
        + tr("PDF文件 (*.pdf);;")
        + tr("OFD文件 (*.ofd);;")
        + tr("Word文档 (*.docx);;")
        + tr("Excel文档 (*.xlsx);;")
        + tr("所有文件 (*)")
    );

    if (!files.isEmpty() && m_fileListView) {
        m_fileListView->addFiles(files);
    }
}

void MainWindow::onActionAddFolder()
{
    QString folder = QFileDialog::getExistingDirectory(
        this,
        tr("选择文件夹"),
        QString(),
        QFileDialog::ShowDirsOnly
    );

    if (!folder.isEmpty() && m_fileListView) {
        m_fileListView->addFolder(folder);
    }
}

void MainWindow::onActionSettings()
{
    SettingsDialog dialog(this);
    dialog.exec();
}

void MainWindow::onActionAbout()
{
    QMessageBox::about(this, tr("关于 PhotoTrick"),
        tr("<h3>PhotoTrick 1.0.0</h3>"
           "<p>图片处理工具 - 发票识别与表格提取</p>"
           "<p>功能：</p>"
           "<ul>"
           "<li>发票/行程单识别，按交通/住宿/餐饮分类</li>"
           "<li>表格提取与合并</li>"
           "<li>支持多种导出格式</li>"
           "</ul>"
           "<p>© 2024 PhotoTrick</p>"));
}

void MainWindow::onProcessingStarted()
{
    if (m_isProcessing) {
        qDebug() << "MainWindow: Already processing, ignoring start request";
        return;
    }

    // Get files to process
    m_pendingFiles = m_fileListView->getFiles();
    if (m_pendingFiles.isEmpty()) {
        m_statusLabel->setText(tr("没有文件需要处理"));
        QMessageBox::information(this, tr("提示"), tr("请先添加要处理的文件"));
        return;
    }

    m_isProcessing = true;
    m_currentFileIndex = 0;

    // Reset batch result table
    resetBatchResultTable();

    m_statusLabel->setText(tr("处理中..."));
    m_progressBar->setValue(0);
    m_progressBar->show();

    // 清空日志并开始记录
    m_processingPanel->clearLog();
    m_processingPanel->appendLog(tr("开始处理 %1 个文件").arg(m_pendingFiles.size()));

    // 检查OCR后端状态
    QString backend = m_processingPanel->ocrBackend();
    m_processingPanel->appendLog(tr("OCR后端: %1").arg(backend));

    qDebug() << "MainWindow: Starting processing of" << m_pendingFiles.size() << "files";

    processNextFile();
}

void MainWindow::processNextFile()
{
    if (!m_isProcessing || m_currentFileIndex >= m_pendingFiles.size()) {
        onProcessingFinished();
        return;
    }

    QString filePath = m_pendingFiles[m_currentFileIndex];
    QString fileName = QFileInfo(filePath).fileName();
    qDebug() << "MainWindow: Processing file" << m_currentFileIndex + 1 << "/" << m_pendingFiles.size() << ":" << filePath;

    m_processingPanel->appendLog(tr("处理文件 (%1/%2): %3")
        .arg(m_currentFileIndex + 1)
        .arg(m_pendingFiles.size())
        .arg(fileName));

    m_statusLabel->setText(tr("处理中: %1 (%2/%3)")
        .arg(fileName)
        .arg(m_currentFileIndex + 1)
        .arg(m_pendingFiles.size()));

    // Get file extension
    QString ext = QFileInfo(filePath).suffix().toLower();

    // List of image formats that QImageReader can handle directly
    QStringList imageFormats = {"jpg", "jpeg", "png", "bmp", "gif", "tiff", "webp"};

    QImage image;
    bool loaded = false;

    if (imageFormats.contains(ext)) {
        // Direct image loading
        m_processingPanel->appendLog(tr("  加载图片..."));
        QImageReader reader(filePath);
        image = reader.read();
        loaded = !image.isNull();
        if (!loaded) {
            QString error = tr("无法读取图片: %1 - %2").arg(filePath).arg(reader.errorString());
            qWarning() << "MainWindow:" << error;
            onProcessingError(error);
            return;
        }
        m_processingPanel->appendLog(tr("  图片尺寸: %1x%2").arg(image.width()).arg(image.height()));
    } else {
        // Use FileConverter for PDF, OFD, DOCX, XLSX, HEIC
        m_processingPanel->appendLog(tr("  转换文件格式 (%1)...").arg(ext.toUpper()));
        m_statusLabel->setText(tr("转换中: %1 (%2/%3)")
            .arg(fileName)
            .arg(m_currentFileIndex + 1)
            .arg(m_pendingFiles.size()));

        // Check if format is supported by converter
        QStringList convertibleFormats = {"pdf", "ofd", "docx", "xlsx", "heic"};
        if (!convertibleFormats.contains(ext)) {
            skipCurrentFile(tr("不支持的文件格式: %1").arg(ext.toUpper()));
            return;
        }

        QList<QImage> images = m_fileConverter->convertToImages(filePath);

        if (images.isEmpty()) {
            skipCurrentFile(tr("格式转换失败，请检查文件是否损坏或转换为PNG/JPG后重试"));
            return;
        }

        // For now, process only the first page/image
        // TODO: Handle multi-page PDFs properly
        image = images.first();
        loaded = true;
        m_processingPanel->appendLog(tr("  转换完成，共 %1 页，处理第1页").arg(images.size()));
        qDebug() << "MainWindow: Converted" << filePath << "to" << images.size() << "image(s)";
    }

    if (!loaded || image.isNull()) {
        onProcessingError(tr("无法加载图片: %1").arg(filePath));
        return;
    }

    // Get processing mode
    ProcessingPanel::ProcessingMode mode = m_processingPanel->processingMode();
    QString modeStr;
    switch (mode) {
    case ProcessingPanel::InvoiceRecognition:
        modeStr = tr("发票识别");
        break;
    case ProcessingPanel::TableExtraction:
        modeStr = tr("表格提取");
        break;
    case ProcessingPanel::ItineraryRecognition:
        modeStr = tr("行程单识别");
        break;
    }
    m_processingPanel->appendLog(tr("  执行: %1").arg(modeStr));

    // Start processing based on mode
    switch (mode) {
    case ProcessingPanel::InvoiceRecognition:
        m_resultPreview->setDisplayMode(ResultPreview::InvoiceMode);
        m_invoiceRecognizer->recognize(image);
        break;
    case ProcessingPanel::TableExtraction:
        m_resultPreview->setDisplayMode(ResultPreview::TableMode);
        m_tableExtractor->extract(image);
        break;
    case ProcessingPanel::ItineraryRecognition:
        m_resultPreview->setDisplayMode(ResultPreview::ItineraryMode);
        m_itineraryRecognizer->recognize(image);
        break;
    }
}

void MainWindow::onProcessingFinished()
{
    m_isProcessing = false;
    m_pendingFiles.clear();
    m_statusLabel->setText(tr("处理完成"));
    m_progressBar->setValue(100);

    m_processingPanel->appendLog(tr("处理完成"));

    // Generate summary report for non-invoice and skipped files
    writeBatchSummaryFile();

    // 隐藏进度条
    QTimer::singleShot(2000, this, [this]() {
        m_progressBar->hide();
    });
}

void MainWindow::onProcessingProgress(int percent)
{
    m_progressBar->setValue(percent);
}

void MainWindow::onOcrBackendChanged(const QString &backend)
{
    OcrManager::Backend ocrBackend = OcrManager::PaddleOCR_Local;

    if (backend == "paddle_local") {
        ocrBackend = OcrManager::PaddleOCR_Local;
    } else if (backend == "claude_format") {
        ocrBackend = OcrManager::Claude_Format;
    } else if (backend == "openai_format") {
        ocrBackend = OcrManager::OpenAI_Format;
    }

    qDebug() << "MainWindow: Setting OCR backend to" << backend;
    m_ocrManager->setBackend(ocrBackend);
}

void MainWindow::onServerStatusChanged(int status)
{
    if (m_processingPanel) {
        m_processingPanel->setServerStatus(static_cast<ProcessingPanel::ServerStatus>(status));
    }

    // Update status bar
    switch (status) {
    case 0: // NotRunning
        m_statusLabel->setText(tr("OCR服务器未启动"));
        break;
    case 1: // Starting
        m_statusLabel->setText(tr("正在启动OCR服务器..."));
        break;
    case 2: // Running
        m_statusLabel->setText(tr("OCR服务器已就绪"));
        break;
    case 3: // Error
        m_statusLabel->setText(tr("OCR服务器错误"));
        break;
    }
}

void MainWindow::onInvoiceRecognized(const InvoiceData &invoice)
{
    qDebug() << "MainWindow: Invoice recognized - " << invoice.invoiceNumber
             << "Amount:" << invoice.totalAmount
             << "Category:" << invoice.categoryString()
             << "IsValid:" << invoice.isValidInvoice;

    // Update progress
    m_progressBar->setValue(static_cast<int>(100.0 * (m_currentFileIndex + 1) / m_pendingFiles.size()));

    QString currentFile = currentProcessingFileName();
    QString fileName = QFileInfo(currentFile).fileName();

    // Check if this is a valid invoice
    if (!invoice.isValidInvoice) {
        // Record non-invoice file with reason
        m_nonInvoiceFiles[currentFile] = invoice.invalidReason.isEmpty()
            ? tr("非发票文档")
            : invoice.invalidReason;
        m_processingPanel->appendLog(tr("  跳过非发票文件: %1 - %2")
            .arg(fileName)
            .arg(m_nonInvoiceFiles[currentFile]));
    } else {
        // Valid invoice - add to results
        m_processingPanel->appendLog(tr("  识别成功: 发票号 %1, 金额 ¥%2, 税额 ¥%3, 税率 %4%, 分类: %5")
            .arg(invoice.invoiceNumber)
            .arg(invoice.totalAmount, 0, 'f', 2)
            .arg(invoice.taxAmount, 0, 'f', 2)
            .arg(invoice.taxRate, 0, 'f', 1)
            .arg(invoice.categoryString()));

        // Display result in result preview with accumulation
        // Note: removed "销方" column due to unstable extraction
        if (m_resultPreview) {
            QStringList headers{
                tr("发票号码"), tr("类型"), tr("金额"), tr("税额"),
                tr("税率"), tr("日期"), tr("备注")
            };

            QString taxRateText = invoice.taxRate > 0.0
                ? QString("%1%").arg(QString::number(invoice.taxRate, 'f', invoice.taxRate < 1.0 ? 2 : 0))
                : tr("-");

            QStringList row;
            row << (invoice.invoiceNumber.trimmed().isEmpty() ? tr("-") : invoice.invoiceNumber.trimmed())
                << invoice.categoryString()
                << QString::number(invoice.totalAmount, 'f', 2)
                << QString::number(invoice.taxAmount, 'f', 2)
                << taxRateText
                << (invoice.invoiceDate.isValid() ? invoice.invoiceDate.toString("yyyy-MM-dd") : tr("-"))
                << currentFile;

            appendBatchResultRow(headers, row);
        }
    }

    // Move to next file
    m_currentFileIndex++;
    processNextFile();
}

void MainWindow::onTableExtracted(const TableData &table)
{
    qDebug() << "MainWindow: Table extracted - Rows:" << table.rows.size();

    // Update progress
    m_progressBar->setValue(static_cast<int>(100.0 * (m_currentFileIndex + 1) / m_pendingFiles.size()));

    // 记录提取结果
    if (table.rows.isEmpty() && table.headers.isEmpty()) {
        m_processingPanel->appendLog(tr("  未检测到表格"));
    } else {
        m_processingPanel->appendLog(tr("  提取成功: %1 行 %2 列")
            .arg(table.rows.size())
            .arg(table.headers.size()));
    }

    // Display result - convert TableData to QStringList format with accumulation
    if (m_resultPreview) {
        QList<QStringList> rows;
        for (const auto &tableRow : table.rows) {
            QStringList row;
            for (const auto &cell : tableRow) {
                row << cell.text;
            }
            rows << row;
        }

        int dataCols = table.headers.size();
        for (const auto &r : rows) dataCols = qMax(dataCols, r.size());
        dataCols = qMax(dataCols, 1);

        if (m_resultHeaders.isEmpty()) {
            m_resultHeaders.clear();
            m_resultHeaders << tr("文件");
            for (int i = 0; i < dataCols; ++i) {
                if (i < table.headers.size() && !table.headers[i].trimmed().isEmpty()) {
                    m_resultHeaders << table.headers[i];
                } else {
                    m_resultHeaders << tr("列%1").arg(i + 1);
                }
            }
        } else {
            int oldCols = qMax(0, m_resultHeaders.size() - 1);
            if (dataCols > oldCols) {
                for (int i = oldCols; i < dataCols; ++i) m_resultHeaders << tr("列%1").arg(i + 1);
                for (QStringList &oldRow : m_resultRows) {
                    while (oldRow.size() < m_resultHeaders.size()) oldRow << QString();
                }
            }
        }

        if (rows.isEmpty()) {
            QStringList row;
            row << currentProcessingFileName() << tr("没有表格");
            while (row.size() < m_resultHeaders.size()) row << QString();
            m_resultRows << row;
        } else {
            for (const QStringList &rawRow : rows) {
                QStringList row;
                row << currentProcessingFileName();
                for (int i = 0; i < m_resultHeaders.size() - 1; ++i) {
                    row << (i < rawRow.size() ? rawRow[i] : QString());
                }
                m_resultRows << row;
            }
        }

        m_resultPreview->setTableData(m_resultHeaders, m_resultRows);
    }

    // Move to next file
    m_currentFileIndex++;
    processNextFile();
}

void MainWindow::onItineraryRecognized(const ItineraryData &itinerary)
{
    qDebug() << "MainWindow: Itinerary recognized - " << itinerary.flightTrainNo
             << "Route:" << itinerary.departure << "->" << itinerary.destination;

    // Update progress
    m_progressBar->setValue(static_cast<int>(100.0 * (m_currentFileIndex + 1) / m_pendingFiles.size()));

    // 记录识别结果
    const bool hasCoreData = !itinerary.flightTrainNo.trimmed().isEmpty()
        || !itinerary.departure.trimmed().isEmpty()
        || !itinerary.destination.trimmed().isEmpty()
        || itinerary.price > 0.0;
    if (hasCoreData) {
        m_processingPanel->appendLog(tr("  识别成功: %1 %2->%3, 金额 ¥%4, 税额 ¥%5")
            .arg(itinerary.flightTrainNo)
            .arg(itinerary.departure)
            .arg(itinerary.destination)
            .arg(itinerary.price, 0, 'f', 2)
            .arg(itinerary.taxAmount, 0, 'f', 2));
    } else {
        m_processingPanel->appendLog(tr("  未识别出有效行程单字段"));
    }

    // Display result with accumulation
    if (m_resultPreview) {
        QStringList headers{
            tr("文件"), tr("类型"), tr("乘客"), tr("出发地"),
            tr("目的地"), tr("航班/车次"), tr("金额"), tr("税额")
        };

        QStringList row;
        row << currentProcessingFileName()
            << itinerary.typeToString()
            << itinerary.passengerName
            << itinerary.departure
            << itinerary.destination
            << itinerary.flightTrainNo
            << QString::number(itinerary.price, 'f', 2)
            << QString::number(itinerary.taxAmount, 'f', 2);

        appendBatchResultRow(headers, row);
    }

    // Move to next file
    m_currentFileIndex++;
    processNextFile();
}

void MainWindow::onProcessingError(const QString &error)
{
    qWarning() << "MainWindow: Processing error:" << error;

    m_statusLabel->setText(tr("错误: %1").arg(error.left(50)));

    // 记录错误
    m_processingPanel->appendLog(tr("  错误: %1").arg(error));

    // Show error to user
    QMessageBox::warning(this, tr("处理错误"), error);

    // Continue with next file
    m_currentFileIndex++;
    processNextFile();
}

void MainWindow::loadApiSettings()
{
    ConfigManager *cfg = ConfigManager::instance();

    // Load API settings to processing panel
    QString apiKey = cfg->apiKey();
    QString baseUrl = cfg->baseUrl();
    QString model = cfg->model();

    if (!apiKey.isEmpty()) {
        m_processingPanel->setApiKey(apiKey);
        m_processingPanel->setBaseUrl(baseUrl);
        m_processingPanel->setModel(model);
        m_processingPanel->setApiStatus(ProcessingPanel::ApiStatus::Configured);
    }
}

void MainWindow::saveApiSettings()
{
    ConfigManager *cfg = ConfigManager::instance();

    cfg->setApiKey(m_processingPanel->apiKey());
    cfg->setBaseUrl(m_processingPanel->baseUrl());
    cfg->setModel(m_processingPanel->model());
    cfg->save();
}

void MainWindow::resetBatchResultTable()
{
    m_resultHeaders.clear();
    m_resultRows.clear();
    m_nonInvoiceFiles.clear();
    m_skippedFiles.clear();
    if (m_resultPreview) {
        m_resultPreview->clearData();
    }
}

QString MainWindow::currentProcessingFileName() const
{
    if (m_currentFileIndex >= 0 && m_currentFileIndex < m_pendingFiles.size()) {
        return m_pendingFiles[m_currentFileIndex];  // Return full path for proper tracking
    }
    return QString();
}

void MainWindow::appendBatchResultRow(const QStringList &headers, const QStringList &row)
{
    if (headers.isEmpty()) return;

    if (m_resultHeaders.isEmpty()) {
        m_resultHeaders = headers;
    }

    if (m_resultHeaders != headers) {
        qWarning() << "MainWindow: result headers mismatch, ignore row";
        return;
    }

    QStringList normalized = row;
    while (normalized.size() < m_resultHeaders.size()) normalized << QString();
    if (normalized.size() > m_resultHeaders.size()) normalized = normalized.mid(0, m_resultHeaders.size());

    m_resultRows.append(normalized);

    if (m_resultPreview) {
        m_resultPreview->setTableData(m_resultHeaders, m_resultRows);
    }
}

void MainWindow::writeBatchSummaryFile()
{
    // Generate summary report for non-invoice and skipped files
    if (m_nonInvoiceFiles.isEmpty() && m_skippedFiles.isEmpty()) {
        return;  // Nothing to report
    }

    QString reportPath = QDir::currentPath() + "/处理报告_" +
        QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".md";

    QFile file(reportPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_processingPanel->appendLog(tr("无法创建处理报告文件"));
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    out << "# 文件处理报告\n\n";
    out << "生成时间: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n\n";

    if (!m_nonInvoiceFiles.isEmpty()) {
        out << "## 非发票文件\n\n";
        out << "以下文件被识别为非发票文档，未计入发票统计：\n\n";
        out << "| 文件名 | 原因 |\n";
        out << "|--------|------|\n";
        for (auto it = m_nonInvoiceFiles.begin(); it != m_nonInvoiceFiles.end(); ++it) {
            out << "| " << it.key() << " | " << it.value() << " |\n";
        }
        out << "\n";
    }

    if (!m_skippedFiles.isEmpty()) {
        out << "## 跳过的文件\n\n";
        out << "以下文件因格式不支持或其他原因被跳过：\n\n";
        out << "| 文件名 | 原因 |\n";
        out << "|--------|------|\n";
        for (auto it = m_skippedFiles.begin(); it != m_skippedFiles.end(); ++it) {
            out << "| " << it.key() << " | " << it.value() << " |\n";
        }
        out << "\n";
    }

    out << "---\n";
    out << "*此报告由 PhotoTrick 自动生成*\n";

    file.close();

    m_processingPanel->appendLog(tr("处理报告已生成: %1").arg(reportPath));

    // Clear tracking for next batch
    m_nonInvoiceFiles.clear();
    m_skippedFiles.clear();
}

void MainWindow::skipCurrentFile(const QString &reason)
{
    QString currentFile = currentProcessingFileName();
    if (!currentFile.isEmpty()) {
        m_skippedFiles[currentFile] = reason;
        QString fileName = QFileInfo(currentFile).fileName();
        m_processingPanel->appendLog(tr("  跳过文件: %1 - %2").arg(fileName).arg(reason));
    }

    m_currentFileIndex++;
    processNextFile();
}
