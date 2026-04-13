#ifndef FILELISTVIEW_H
#define FILELISTVIEW_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QMap>
#include <QMenu>

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
    void loadRecentFiles();
    void saveRecentFiles();

signals:
    void filesAdded(int count);
    void fileRemoved(const QString &path);
    void selectionChanged(const QString &path);

private slots:
    void onRemoveSelected();
    void onClearAll();
    void onItemSelectionChanged();
    void onRecentFileTriggered();

private:
    void setupUI();
    void setupConnections();
    bool isFileSupported(const QString &filePath) const;
    QString getFileTypeIcon(const QString &filePath) const;
    void addFileItem(const QString &filePath);
    void updateRecentMenu();

    QListWidget *m_listWidget;
    QPushButton *m_btnAddFiles;
    QPushButton *m_btnAddFolder;
    QPushButton *m_btnRemove;
    QPushButton *m_btnClear;
    QLabel *m_labelCount;
    QMenu *m_recentMenu;

    QMap<QString, QString> m_filePaths; // display name -> full path
};

#endif // FILELISTVIEW_H
