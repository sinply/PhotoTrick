#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "ui/FileListView.h"
#include "ui/ProcessingPanel.h"
#include "ui/ResultPreview.h"
#include "ui/CategoryView.h"
#include "ui/SettingsDialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QSplitter>

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
{
    ui->setupUi(this);

    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupCentralWidget();
    setupConnections();
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
        m_statusLabel->setText(tr("已取消"));
        m_progressBar->hide();
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
    m_statusLabel->setText(tr("处理中..."));
    m_progressBar->setValue(0);
    m_progressBar->show();
}

void MainWindow::onProcessingFinished()
{
    m_statusLabel->setText(tr("处理完成"));
    m_progressBar->setValue(100);
    m_progressBar->hide();
}

void MainWindow::onProcessingProgress(int percent)
{
    m_progressBar->setValue(percent);
}
