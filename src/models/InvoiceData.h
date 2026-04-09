#ifndef INVOICEDATA_H
#define INVOICEDATA_H

#include <QString>
#include <QDate>
#include <QList>
#include <QJsonObject>

struct InvoiceItem {
    QString description;
    double quantity = 0.0;
    double unitPrice = 0.0;
    double amount = 0.0;
    double taxRate = 0.0;
    double taxAmount = 0.0;

    QJsonObject toJson() const;
    static InvoiceItem fromJson(const QJsonObject &json);
};

struct InvoiceData {
    // Category
    enum Category {
        Transportation,
        Accommodation,
        Dining,
        Other
    };

    // Basic info
    QString invoiceType;        // 发票类型
    Category category = Other;  // 分类
    QString invoiceNumber;      // 发票号码
    QDate invoiceDate;          // 开票日期

    // Amount info
    double totalAmount = 0.0;       // 价税合计
    double amountWithoutTax = 0.0;  // 不含税金额
    double taxAmount = 0.0;         // 税额
    double taxRate = 0.0;           // 税率(%)
    QString currency = "CNY";       // 币种

    // Seller/Buyer
    QString sellerName;     // 销方名称
    QString sellerTaxId;    // 销方税号
    QString buyerName;      // 购方名称
    QString buyerTaxId;     // 购方税号

    // Transportation specific
    QString departure;      // 出发地
    QString destination;    // 目的地
    QString passengerName;  // 乘客姓名

    // Accommodation specific
    int stayDays = 0;       // 入住天数

    // Items
    QList<InvoiceItem> items;

    // Confidence
    double confidence = 0.0;

    // Source
    QString sourceFile;     // 源文件路径

    // Serialization
    QJsonObject toJson() const;
    static InvoiceData fromJson(const QJsonObject &json);

    // Helper
    QString categoryString() const;
    static Category categoryFromString(const QString &str);
};

#endif // INVOICEDATA_H
