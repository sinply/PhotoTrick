#include "SettingsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QFileDialog>
#include <QSettings>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_tabWidget(nullptr)
    , m_comboLanguage(nullptr)
    , m_comboTheme(nullptr)
    , m_editOutputDir(nullptr)
    , m_comboDefaultBackend(nullptr)
    , m_comboDefaultFormat(nullptr)
{
    setupUI();
    loadSettings();
}

void SettingsDialog::setupUI()
{
    setWindowTitle(tr("设置"));
    setMinimumSize(450, 350);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createGeneralTab(), tr("常规"));
    m_tabWidget->addTab(createOcrTab(), tr("OCR"));
    m_tabWidget->addTab(createExportTab(), tr("导出"));
    mainLayout->addWidget(m_tabWidget);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    QPushButton *btnOk = new QPushButton(tr("确定"), this);
    QPushButton *btnCancel = new QPushButton(tr("取消"), this);
    btnCancel->setProperty("secondary", true);

    buttonLayout->addWidget(btnOk);
    buttonLayout->addWidget(btnCancel);
    mainLayout->addLayout(buttonLayout);

    connect(btnOk, &QPushButton::clicked, this, &SettingsDialog::onAccepted);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

QWidget *SettingsDialog::createGeneralTab()
{
    QWidget *tab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(tab);

    QFormLayout *form = new QFormLayout();

    m_comboLanguage = new QComboBox(this);
    m_comboLanguage->addItem(tr("简体中文"), "zh_CN");
    m_comboLanguage->addItem(tr("English"), "en_US");
    form->addRow(tr("语言:"), m_comboLanguage);

    m_comboTheme = new QComboBox(this);
    m_comboTheme->addItem(tr("浅色"), "light");
    m_comboTheme->addItem(tr("深色"), "dark");
    form->addRow(tr("主题:"), m_comboTheme);

    QHBoxLayout *outputLayout = new QHBoxLayout();
    m_editOutputDir = new QLineEdit(this);
    QPushButton *btnBrowse = new QPushButton(tr("浏览..."), this);
    btnBrowse->setProperty("secondary", true);
    outputLayout->addWidget(m_editOutputDir);
    outputLayout->addWidget(btnBrowse);
    form->addRow(tr("输出目录:"), outputLayout);

    connect(btnBrowse, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr("选择输出目录"));
        if (!dir.isEmpty()) {
            m_editOutputDir->setText(dir);
        }
    });

    layout->addLayout(form);
    layout->addStretch();

    return tab;
}

QWidget *SettingsDialog::createOcrTab()
{
    QWidget *tab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(tab);

    QFormLayout *form = new QFormLayout();

    m_comboDefaultBackend = new QComboBox(this);
    m_comboDefaultBackend->addItem(tr("PaddleOCR本地"), "paddle_local");
    m_comboDefaultBackend->addItem(tr("Claude兼容格式"), "claude_format");
    m_comboDefaultBackend->addItem(tr("OpenAI兼容格式"), "openai_format");
    form->addRow(tr("默认OCR后端:"), m_comboDefaultBackend);

    layout->addLayout(form);
    layout->addStretch();

    return tab;
}

QWidget *SettingsDialog::createExportTab()
{
    QWidget *tab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(tab);

    QFormLayout *form = new QFormLayout();

    m_comboDefaultFormat = new QComboBox(this);
    m_comboDefaultFormat->addItem(tr("Markdown"), "markdown");
    m_comboDefaultFormat->addItem(tr("Word"), "word");
    m_comboDefaultFormat->addItem(tr("Excel"), "excel");
    m_comboDefaultFormat->addItem(tr("JSON"), "json");
    form->addRow(tr("默认导出格式:"), m_comboDefaultFormat);

    layout->addLayout(form);
    layout->addStretch();

    return tab;
}

void SettingsDialog::loadSettings()
{
    QSettings settings("PhotoTrick", "PhotoTrick");

    QString lang = settings.value("language", "zh_CN").toString();
    int langIndex = m_comboLanguage->findData(lang);
    if (langIndex >= 0) m_comboLanguage->setCurrentIndex(langIndex);

    QString theme = settings.value("theme", "light").toString();
    int themeIndex = m_comboTheme->findData(theme);
    if (themeIndex >= 0) m_comboTheme->setCurrentIndex(themeIndex);

    m_editOutputDir->setText(settings.value("outputDir", QDir::homePath()).toString());

    QString backend = settings.value("ocrBackend", "paddle_local").toString();
    int backendIndex = m_comboDefaultBackend->findData(backend);
    if (backendIndex >= 0) m_comboDefaultBackend->setCurrentIndex(backendIndex);

    QString format = settings.value("exportFormat", "markdown").toString();
    int formatIndex = m_comboDefaultFormat->findData(format);
    if (formatIndex >= 0) m_comboDefaultFormat->setCurrentIndex(formatIndex);
}

void SettingsDialog::saveSettings()
{
    QSettings settings("PhotoTrick", "PhotoTrick");

    settings.setValue("language", m_comboLanguage->currentData());
    settings.setValue("theme", m_comboTheme->currentData());
    settings.setValue("outputDir", m_editOutputDir->text());
    settings.setValue("ocrBackend", m_comboDefaultBackend->currentData());
    settings.setValue("exportFormat", m_comboDefaultFormat->currentData());
}

void SettingsDialog::onAccepted()
{
    saveSettings();
    accept();
}
