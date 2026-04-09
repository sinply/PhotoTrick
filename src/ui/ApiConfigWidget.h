#ifndef APICONFIGWIDGET_H
#define APICONFIGWIDGET_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>

class ApiConfigWidget : public QDialog
{
    Q_OBJECT

public:
    explicit ApiConfigWidget(QWidget *parent = nullptr);

    void setApiKey(const QString &key);
    void setBaseUrl(const QString &url);
    void setModel(const QString &model);

    QString apiKey() const;
    QString baseUrl() const;
    QString model() const;

private slots:
    void onFormatChanged(int index);
    void onProviderChanged(int index);
    void onTestConnection();

private:
    void setupUI();
    void setupConnections();

    QComboBox *m_comboFormat;
    QComboBox *m_comboProvider;
    QLineEdit *m_editApiKey;
    QLineEdit *m_editBaseUrl;
    QComboBox *m_comboModel;
    QPushButton *m_btnTest;
};

#endif // APICONFIGWIDGET_H
