#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

private slots:
    void onAccepted();

private:
    void setupUI();
    void loadSettings();
    void saveSettings();

    QWidget *createGeneralTab();
    QWidget *createOcrTab();
    QWidget *createExportTab();

    QTabWidget *m_tabWidget;

    // General tab
    QComboBox *m_comboLanguage;
    QComboBox *m_comboTheme;
    QLineEdit *m_editOutputDir;

    // OCR tab
    QComboBox *m_comboDefaultBackend;

    // Export tab
    QComboBox *m_comboDefaultFormat;
};

#endif // SETTINGSDIALOG_H
