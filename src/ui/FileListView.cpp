#include "FileListView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>

FileListView::FileListView(QWidget *parent)
    : QWidget(parent)
    , m_listWidget(nullptr)
    , m_btnAddFiles(nullptr)
    , m_btnAddFolder(nullptr)
    , m_btnRemove(nullptr)
    , m_btnClear(nullptr)
    , m_labelCount(nullptr)
{
    setupUI();
    setupConnections();
}

void FileListView::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);

    // Toolbar
    QHBoxLayout *toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(5);

    m_btnAddFiles = new QPushButton(tr("添加文件"), this);
    m_btnAddFolder = new QPushButton(tr("添加文件夹"), this);
    m_btnRemove = new QPushButton(tr("删除"), this);
    m_btnClear = new QPushButton(tr("清空"), this);

    m_btnRemove->setProperty("secondary", true);
    m_btnClear->setProperty("secondary", true);

    toolbarLayout->addWidget(m_btnAddFiles);
    toolbarLayout->addWidget(m_btnAddFolder);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(m_btnRemove);
    toolbarLayout->addWidget(m_btnClear);

    mainLayout->addLayout(toolbarLayout);

    // List widget
    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listWidget->setAlternatingRowColors(true);
    mainLayout->addWidget(m_listWidget);

    // Status label
    m_labelCount = new QLabel(tr("共 0 个文件"), this);
    mainLayout->addWidget(m_labelCount);
}

void FileListView::setupConnections()
{
    connect(m_btnAddFiles, &QPushButton::clicked, this, [this]() {
        QStringList files = QFileDialog::getOpenFileNames(
            this, tr("选择文件"), QString(),
            tr("支持的文件 (*.jpg *.jpeg *.png *.bmp *.gif *.tiff *.webp *.heic *.pdf *.ofd *.docx *.xlsx)")
        );
        if (!files.isEmpty()) {
            addFiles(files);
        }
    });

    connect(m_btnAddFolder, &QPushButton::clicked, this, [this]() {
        QString folder = QFileDialog::getExistingDirectory(this, tr("选择文件夹"));
        if (!folder.isEmpty()) {
            addFolder(folder);
        }
    });

    connect(m_btnRemove, &QPushButton::clicked, this, &FileListView::onRemoveSelected);
    connect(m_btnClear, &QPushButton::clicked, this, &FileListView::onClearAll);
    connect(m_listWidget, &QListWidget::itemSelectionChanged, this, &FileListView::onItemSelectionChanged);
}

void FileListView::addFiles(const QStringList &files)
{
    int addedCount = 0;
    for (const QString &file : files) {
        if (isFileSupported(file) && !m_filePaths.values().contains(file)) {
            addFileItem(file);
            addedCount++;
        }
    }

    if (addedCount > 0) {
        m_labelCount->setText(tr("共 %1 个文件").arg(m_filePaths.size()));
        emit filesAdded(addedCount);
    }
}

void FileListView::addFolder(const QString &folder)
{
    QDir dir(folder);
    QStringList filters;
    filters << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp" << "*.gif"
            << "*.tiff" << "*.webp" << "*.heic" << "*.pdf" << "*.ofd"
            << "*.docx" << "*.xlsx";

    QStringList files;
    QDirIterator it(folder, filters, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        files << it.next();
    }

    addFiles(files);
}

void FileListView::clearFiles()
{
    m_listWidget->clear();
    m_filePaths.clear();
    m_labelCount->setText(tr("共 0 个文件"));
}

int FileListView::fileCount() const
{
    return m_filePaths.size();
}

QStringList FileListView::getFiles() const
{
    return m_filePaths.values();
}

void FileListView::onRemoveSelected()
{
    QList<QListWidgetItem*> selected = m_listWidget->selectedItems();
    for (QListWidgetItem *item : selected) {
        QString displayText = item->text().split('\n').first();
        QString filePath = m_filePaths.value(displayText);
        m_filePaths.remove(displayText);
        emit fileRemoved(filePath);
        delete item;
    }
    m_labelCount->setText(tr("共 %1 个文件").arg(m_filePaths.size()));
}

void FileListView::onClearAll()
{
    if (m_filePaths.isEmpty()) return;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("确认清空"), tr("确定要清空所有文件吗？"),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        clearFiles();
    }
}

void FileListView::onItemSelectionChanged()
{
    QList<QListWidgetItem*> selected = m_listWidget->selectedItems();
    if (selected.size() == 1) {
        QString displayText = selected.first()->text().split('\n').first();
        QString filePath = m_filePaths.value(displayText);
        emit selectionChanged(filePath);
    }
}

bool FileListView::isFileSupported(const QString &filePath) const
{
    QString ext = QFileInfo(filePath).suffix().toLower();
    QStringList supported = {"jpg", "jpeg", "png", "bmp", "gif", "tiff",
                             "webp", "heic", "pdf", "ofd", "docx", "xlsx"};
    return supported.contains(ext);
}

QString FileListView::getFileTypeIcon(const QString &filePath) const
{
    QString ext = QFileInfo(filePath).suffix().toLower();

    if (ext == "pdf") return "📄";
    if (ext == "ofd") return "📄";
    if (ext == "docx") return "📝";
    if (ext == "xlsx") return "📊";
    if (ext == "heic") return "🖼️";

    return "🖼️"; // Default image icon
}

void FileListView::addFileItem(const QString &filePath)
{
    QFileInfo info(filePath);
    QString displayText = QString("%1\n%2")
        .arg(info.fileName())
        .arg(info.size() / 1024 >= 1024
             ? QString("%1 MB").arg(info.size() / 1024 / 1024.0, 0, 'f', 1)
             : QString("%1 KB").arg(info.size() / 1024));

    QListWidgetItem *item = new QListWidgetItem(m_listWidget);
    item->setText(getFileTypeIcon(filePath) + " " + displayText);
    item->setData(Qt::UserRole, filePath);
    item->setToolTip(filePath);

    m_filePaths[info.fileName()] = filePath;
}
