#ifndef APICONFIGWIDGET_H
#define APICONFIGWIDGET_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class ApiConfigWidget : public QDialog
{
    Q_OBJECT

public:
    explicit ApiConfigWidget(const QString &backend, QWidget *parent = nullptr);

    void setApiKey(const QString &key);
    void setBaseUrl(const QString &url);
    void setModel(const QString &model);
    void setProvider(const QString &provider);

    QString apiKey() const;
    QString baseUrl() const;
    QString model() const;
    QString provider() const;

    enum class ApiStatus {
        NotConfigured,
        Configured,
        Testing,
        Valid,
        Invalid
    };

    void setApiStatus(ApiStatus status, const QString &message = QString());

private slots:
    void onProviderChanged(int index);
    void onTestConnection();
    void onSave();

private:
    void setupUI();
    void setupConnections();
    void loadSettings();
    void saveSettings();

    QString m_backend;
    QComboBox *m_comboProvider;
    QLineEdit *m_editApiKey;
    QLineEdit *m_editBaseUrl;
    QComboBox *m_comboModel;
    QLineEdit *m_editModel;
    QPushButton *m_btnTest;
    QLabel *m_labelApiStatus;
};

#endif // APICONFIGWIDGET_H
