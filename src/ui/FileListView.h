#ifndef FILELISTVIEW_H
#define FILELISTVIEW_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QMap>

class FileListView : public QWidget
{
    Q_OBJECT

public:
    explicit FileListView(QWidget *parent = nullptr);

    void addFiles(const QStringList &files);
    void addFolder(const QString &folder);
    void clearFiles();
    int fileCount() const;
    QStringList getFiles() const;

signals:
    void filesAdded(int count);
    void fileRemoved(const QString &path);
    void selectionChanged(const QString &path);

private slots:
    void onRemoveSelected();
    void onClearAll();
    void onItemSelectionChanged();

private:
    void setupUI();
    void setupConnections();
    bool isFileSupported(const QString &filePath) const;
    QString getFileTypeIcon(const QString &filePath) const;
    void addFileItem(const QString &filePath);

    QListWidget *m_listWidget;
    QPushButton *m_btnAddFiles;
    QPushButton *m_btnAddFolder;
    QPushButton *m_btnRemove;
    QPushButton *m_btnClear;
    QLabel *m_labelCount;

    QMap<QString, QString> m_filePaths; // display name -> full path
};

#endif // FILELISTVIEW_H
