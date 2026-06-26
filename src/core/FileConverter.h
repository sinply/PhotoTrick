#ifndef FILECONVERTER_H
#define FILECONVERTER_H

#include <QObject>
#include <QImage>
#include <QProcess>
#include <QPointer>

class FileConverter : public QObject
{
    Q_OBJECT

public:
    explicit FileConverter(QObject *parent = nullptr);

    // 同步接口（FileManager 等仍用，会阻塞调用线程）
    bool convertToImage(const QString &filePath, QImage &outImage);
    QList<QImage> convertToImages(const QString &filePath);

    bool convertHeic(const QString &filePath, QImage &outImage);
    bool convertPdf(const QString &filePath, QList<QImage> &outImages);
    bool convertOfd(const QString &filePath, QList<QImage> &outImages);
    bool extractImagesFromDocx(const QString &filePath, QList<QImage> &outImages);
    bool extractImagesFromXlsx(const QString &filePath, QList<QImage> &outImages);

    void setPythonPath(const QString &path);
    void setConverterScriptPath(const QString &path);

    // 异步接口（主流程用，不阻塞 UI，通过信号返回结果）
    bool requestConvert(const QString &filePath);
    void cancel();
    bool isBusy() const { return m_busy; }

signals:
    void conversionFinished(const QString &filePath, const QList<QImage> &images);
    void conversionError(const QString &filePath, const QString &error);
    void conversionLog(const QString &msg);

private:
    bool runPythonConverter(const QString &action, const QString &filePath, const QString &outputPath);

    // 异步辅助
    void startAsyncConvert(const QString &filePath, const QString &action,
                           const QString &outputPath, bool outputIsFile);
    void collectImagesFromDir(const QString &dir, QList<QImage> &out);
    void cleanupPath(const QString &path, bool isFile);
    void finishAsync(bool success, const QString &errorMessage);

    QString m_pythonPath;
    QString m_scriptPath;

    // 异步状态
    QPointer<QProcess> m_process;
    QString m_currentFilePath;
    QString m_currentOutputPath;
    bool m_currentOutputIsFile = false;
    bool m_busy = false;
};

#endif // FILECONVERTER_H
