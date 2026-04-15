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
class OcrManager;
class InvoiceRecognizer;
class TableExtractor;
class ItineraryRecognizer;
class FileConverter;

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
    void onOcrBackendChanged(const QString &backend);
    void onServerStatusChanged(int status);

    // Processing slots
    void onInvoiceRecognized(const class InvoiceData &invoice);
    void onTableExtracted(const class TableData &table);
    void onItineraryRecognized(const class ItineraryData &itinerary);
    void onProcessingError(const QString &error);
    void processNextFile();

private:
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupCentralWidget();
    void setupConnections();
    void loadApiSettings();
    void loadApiSettingsForBackend(const QString &backend);
    void saveApiSettings();

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

    // OCR Manager
    OcrManager *m_ocrManager;

    // Processors
    InvoiceRecognizer *m_invoiceRecognizer;
    TableExtractor *m_tableExtractor;
    ItineraryRecognizer *m_itineraryRecognizer;

    // Processing state
    QStringList m_pendingFiles;
    int m_currentFileIndex;
    bool m_isProcessing;

    // Batch result accumulation
    QStringList m_resultHeaders;
    QList<QStringList> m_resultRows;

    // Non-invoice and skipped files tracking
    QMap<QString, QString> m_nonInvoiceFiles;  // file -> reason
    QMap<QString, QString> m_skippedFiles;     // file -> reason

    // File converter for PDF/OFD/DOCX/XLSX
    FileConverter *m_fileConverter;

    // Batch result helpers
    void resetBatchResultTable();
    void appendBatchResultRow(const QStringList &headers, const QStringList &row);
    QString currentProcessingFileName() const;
    void writeBatchSummaryFile();
    void skipCurrentFile(const QString &reason);
 };

#endif // MAINWINDOW_H
