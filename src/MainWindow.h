#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QTabWidget>
#include <QLabel>
#include <QProgressBar>

// Forward declarations
class FileListView;
class ProcessingPanel;
class ResultPreview;
class CategoryView;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onActionAddFiles();
    void onActionAddFolder();
    void onActionSettings();
    void onActionAbout();
    void onProcessingStarted();
    void onProcessingFinished();
    void onProcessingProgress(int percent);

private:
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupCentralWidget();
    void setupConnections();

    Ui::MainWindow *ui;

    // Central widgets
    QSplitter *m_mainSplitter;
    QSplitter *m_rightSplitter;

    // Custom widgets
    FileListView *m_fileListView;
    ProcessingPanel *m_processingPanel;
    ResultPreview *m_resultPreview;
    CategoryView *m_categoryView;

    // Status bar widgets
    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;
};

#endif // MAINWINDOW_H
