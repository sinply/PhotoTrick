#include "InvoiceRecognizer.h"
#include "../core/OcrManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QtConcurrent>

const QString InvoiceRecognizer::INVOICE_PROMPT = R"(
请分析这张发票图片，提取以下信息并以JSON格式返回：

{
    "invoiceType": "发票类型（增值税专用发票/增值税普通发票/电子发票等）",
    "invoiceNumber": "发票号码",
    "date": "开票日期（YYYY-MM-DD格式）",
    "buyerName": "购买方名称",
    "buyerTaxId": "购买方税号",
    "sellerName": "销售方名称",
    "sellerTaxId": "销售方税号",
    "totalAmount": "价税合计（数字）",
    "amountWithoutTax": "不含税金额（数字）",
    "taxAmount": "税额（数字）",
    "taxRate": "税率（数字，如6表示6%）",
    "category": "分类（交通/住宿/餐饮/其他）",
    "departure": "出发地（交通类发票）",
    "destination": "目的地（交通类发票）",
    "passengerName": "乘客姓名",
    "stayDays": "入住天数（住宿类发票）",
    "items": [
        {
            "description": "货物/服务名称",
            "quantity": "数量",
            "unitPrice": "单价",
            "amount": "金额"
        }
    ]
}

请确保返回有效的JSON格式。如果某字段无法识别，请填null。
)";

InvoiceRecognizer::InvoiceRecognizer(QObject *parent)
    : QObject(parent)
    , m_ocrManager(nullptr)
{
}

void InvoiceRecognizer::setOcrManager(OcrManager *manager)
{
    if (m_ocrManager) {
        disconnect(m_ocrManager, nullptr, this, nullptr);
    }
    m_ocrManager = manager;
    if (m_ocrManager) {
        connect(m_ocrManager, &OcrManager::recognitionFinished,
                this, &InvoiceRecognizer::onOcrFinished);
        connect(m_ocrManager, &OcrManager::recognitionError,
                this, &InvoiceRecognizer::recognitionError);
    }
}

void InvoiceRecognizer::recognize(const QImage &image)
{
    if (!m_ocrManager) {
        emit recognitionError(tr("OCR管理器未设置"));
        return;
    }
    m_ocrManager->recognizeImage(image, INVOICE_PROMPT);
}

void InvoiceRecognizer::recognizeAsync(const QImage &image)
{
    if (!m_ocrManager) {
        emit recognitionError(tr("OCR管理器未设置"));
        return;
    }

    QtConcurrent::run([this, image]() {
        recognize(image);
    });
}

void InvoiceRecognizer::onOcrFinished(const QJsonObject &result)
{
    InvoiceData invoice = parseInvoiceData(result);
    emit recognitionFinished(invoice);
}

InvoiceData InvoiceRecognizer::parseInvoiceData(const QJsonObject &json)
{
    InvoiceData invoice;

    // 基础信息
    invoice.invoiceType = json["invoiceType"].toString();
    invoice.invoiceNumber = json["invoiceNumber"].toString();

    // 日期
    QString dateStr = json["date"].toString();
    if (!dateStr.isEmpty()) {
        invoice.invoiceDate = QDate::fromString(dateStr, "yyyy-MM-dd");
    }

    // 购买方信息
    invoice.buyerName = json["buyerName"].toString();
    invoice.buyerTaxId = json["buyerTaxId"].toString();

    // 销售方信息
    invoice.sellerName = json["sellerName"].toString();
    invoice.sellerTaxId = json["sellerTaxId"].toString();

    // 金额信息
    invoice.totalAmount = json["totalAmount"].toDouble(0.0);
    invoice.amountWithoutTax = json["amountWithoutTax"].toDouble(0.0);
    invoice.taxAmount = json["taxAmount"].toDouble(0.0);
    invoice.taxRate = json["taxRate"].toDouble(0.0);

    // 分类
    invoice.category = InvoiceData::categoryFromString(json["category"].toString());

    // 行程信息
    invoice.departure = json["departure"].toString();
    invoice.destination = json["destination"].toString();
    invoice.passengerName = json["passengerName"].toString();
    invoice.stayDays = json["stayDays"].toInt(0);

    // 明细项目
    if (json.contains("items") && json["items"].isArray()) {
        QJsonArray items = json["items"].toArray();
        for (const auto &item : items) {
            QJsonObject itemObj = item.toObject();
            InvoiceItem invItem;
            invItem.description = itemObj["description"].toString();
            if (invItem.description.isEmpty()) {
                invItem.description = itemObj["name"].toString();
            }
            invItem.quantity = itemObj["quantity"].toDouble(1.0);
            invItem.unitPrice = itemObj["unitPrice"].toDouble(0.0);
            invItem.amount = itemObj["amount"].toDouble(0.0);
            invoice.items.append(invItem);
        }
    }

    return invoice;
}
