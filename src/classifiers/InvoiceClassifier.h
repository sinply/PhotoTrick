#ifndef INVOICECLASSIFIER_H
#define INVOICECLASSIFIER_H

#include <QObject>
#include "../models/InvoiceData.h"

class InvoiceClassifier : public QObject
{
    Q_OBJECT

public:
    explicit InvoiceClassifier(QObject *parent = nullptr);

    // 根据发票内容自动分类
    InvoiceData::Category classify(const InvoiceData &invoice);

    // 分类名称转换
    static QString categoryToString(InvoiceData::Category category);
    static InvoiceData::Category stringToCategory(const QString &str);

private:
    void initKeywords();
    InvoiceData::Category matchByKeywords(const QString &text);
    InvoiceData::Category matchBySellerName(const QString &sellerName);

    // 关键词映射
    QMap<InvoiceData::Category, QStringList> m_keywords;
};

#endif // INVOICECLASSIFIER_H
