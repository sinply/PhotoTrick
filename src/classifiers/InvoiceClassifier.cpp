#include "InvoiceClassifier.h"

InvoiceClassifier::InvoiceClassifier(QObject *parent)
    : QObject(parent)
{
    initKeywords();
}

void InvoiceClassifier::initKeywords()
{
    // 交通类关键词
    m_keywords[InvoiceData::Transportation] = {
        QStringLiteral("航空"), QStringLiteral("机票"), QStringLiteral("航班"),
        QStringLiteral("铁路"), QStringLiteral("火车"), QStringLiteral("高铁"),
        QStringLiteral("汽车站"), QStringLiteral("客运站"), QStringLiteral("长途汽车"),
        QStringLiteral("出租车"), QStringLiteral("打车"), QStringLiteral("滴滴"),
        QStringLiteral("网约车"), QStringLiteral("租车"), QStringLiteral("停车场"),
        QStringLiteral("加油站"), QStringLiteral("过路费"), QStringLiteral("高速费"),
        QStringLiteral("机票代理"), QStringLiteral("票务")
    };

    // 住宿类关键词
    m_keywords[InvoiceData::Accommodation] = {
        QStringLiteral("酒店"), QStringLiteral("宾馆"), QStringLiteral("旅馆"),
        QStringLiteral("民宿"), QStringLiteral("招待所"), QStringLiteral("客栈"),
        QStringLiteral("公寓"), QStringLiteral("住宿"), QStringLiteral("房费"),
        QStringLiteral("住宿费"), QStringLiteral("住宿服务")
    };

    // 餐饮类关键词
    m_keywords[InvoiceData::Dining] = {
        QStringLiteral("餐饮"), QStringLiteral("餐厅"), QStringLiteral("饭店"),
        QStringLiteral("酒店"), QStringLiteral("美食"), QStringLiteral("快餐"),
        QStringLiteral("外卖"), QStringLiteral("食品"), QStringLiteral("饮品"),
        QStringLiteral("咖啡"), QStringLiteral("茶楼"), QStringLiteral("小吃"),
        QStringLiteral("火锅"), QStringLiteral("烧烤"), QStringLiteral("海鲜"),
        QStringLiteral("中餐"), QStringLiteral("西餐"), QStringLiteral("日料"),
        QStringLiteral("酒楼"), QStringLiteral("食府")
    };
}

InvoiceData::Category InvoiceClassifier::classify(const InvoiceData &invoice)
{
    // 如果已有分类，验证是否合理
    if (invoice.category != InvoiceData::Other) {
        return invoice.category;
    }

    // 优先根据销售方名称匹配
    InvoiceData::Category category = matchBySellerName(invoice.sellerName);
    if (category != InvoiceData::Other) {
        return category;
    }

    // 根据发票类型判断
    QString invoiceType = invoice.invoiceType;
    if (invoiceType.contains(QStringLiteral("航空")) ||
        invoiceType.contains(QStringLiteral("机票"))) {
        return InvoiceData::Transportation;
    }

    // 根据明细项目判断
    for (const auto &item : invoice.items) {
        category = matchByKeywords(item.description);
        if (category != InvoiceData::Other) {
            return category;
        }
    }

    // 默认其他
    return InvoiceData::Other;
}

InvoiceData::Category InvoiceClassifier::matchByKeywords(const QString &text)
{
    if (text.isEmpty()) {
        return InvoiceData::Other;
    }

    for (auto it = m_keywords.begin(); it != m_keywords.end(); ++it) {
        for (const QString &keyword : it.value()) {
            if (text.contains(keyword)) {
                return it.key();
            }
        }
    }

    return InvoiceData::Other;
}

InvoiceData::Category InvoiceClassifier::matchBySellerName(const QString &sellerName)
{
    if (sellerName.isEmpty()) {
        return InvoiceData::Other;
    }

    // 特殊匹配规则

    // 航空公司
    QStringList airlines = {
        QStringLiteral("航空"), QStringLiteral("机场")
    };
    for (const QString &keyword : airlines) {
        if (sellerName.contains(keyword)) {
            return InvoiceData::Transportation;
        }
    }

    // 铁路
    if (sellerName.contains(QStringLiteral("铁路")) ||
        sellerName.contains(QStringLiteral("高铁"))) {
        return InvoiceData::Transportation;
    }

    // 酒店/住宿
    QStringList hotels = {
        QStringLiteral("酒店"), QStringLiteral("宾馆"),
        QStringLiteral("旅馆"), QStringLiteral("民宿"),
        QStringLiteral("客栈"), QStringLiteral("公寓")
    };
    for (const QString &keyword : hotels) {
        if (sellerName.contains(keyword)) {
            // 注意：酒店餐饮需要进一步判断
            // 如果发票金额较小且包含餐饮关键词，可能是餐饮
            return InvoiceData::Accommodation;
        }
    }

    // 餐饮
    return matchByKeywords(sellerName);
}

QString InvoiceClassifier::categoryToString(InvoiceData::Category category)
{
    switch (category) {
    case InvoiceData::Transportation:
        return QStringLiteral("交通");
    case InvoiceData::Accommodation:
        return QStringLiteral("住宿");
    case InvoiceData::Dining:
        return QStringLiteral("餐饮");
    default:
        return QStringLiteral("其他");
    }
}

InvoiceData::Category InvoiceClassifier::stringToCategory(const QString &str)
{
    if (str == QStringLiteral("交通")) {
        return InvoiceData::Transportation;
    } else if (str == QStringLiteral("住宿")) {
        return InvoiceData::Accommodation;
    } else if (str == QStringLiteral("餐饮")) {
        return InvoiceData::Dining;
    }
    return InvoiceData::Other;
}
