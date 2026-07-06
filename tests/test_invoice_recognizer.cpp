#include <QtTest/QtTest>
#include <QJsonObject>
#include <QJsonDocument>
#include <QRegularExpression>
#include "../src/processors/InvoiceRecognizer.h"
#include "../src/models/InvoiceData.h"
#include "../src/utils/OcrParser.h"

class TestInvoiceRecognizer : public QObject
{
    Q_OBJECT
private:
    InvoiceRecognizer m_recognizer;

    InvoiceData parseRawText(const QString &rawText)
    {
        QJsonObject json;
        json["text"] = rawText;
        return m_recognizer.parseInvoiceDataForTest(json);
    }

    InvoiceData parseStructured(const QJsonObject &obj)
    {
        return m_recognizer.parseInvoiceDataForTest(obj);
    }

private slots:
    void testOcrParser_normalizeKey_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QString>("expected");
        QTest::newRow("lowercase") << "InvoiceNumber" << "invoicenumber";
        QTest::newRow("strip_underscore") << "invoice_number" << "invoicenumber";
        QTest::newRow("strip_dash") << "invoice-number" << "invoicenumber";
        QTest::newRow("strip_space") << "invoice number" << "invoicenumber";
    }

    void testOcrParser_normalizeKey()
    {
        QFETCH(QString, input);
        QFETCH(QString, expected);
        QCOMPARE(OcrParser::normalizeKey(input), expected);
    }

    void testOcrParser_parseNumber()
    {
        QCOMPARE(OcrParser::parseNumber(QJsonValue(123.45)), 123.45);
        QCOMPARE(OcrParser::parseNumber(QJsonValue("¥1,234.56")), 1234.56);
        QCOMPARE(OcrParser::parseNumber(QJsonValue("￥100")), 100.0);
        QCOMPARE(OcrParser::parseNumber(QJsonValue("")), 0.0);
        QCOMPARE(OcrParser::parseNumber(QJsonValue("N/A")), 0.0);
    }

    void testOcrParser_extractRawText()
    {
        QJsonObject json;
        json["text"] = "hello";
        QCOMPARE(OcrParser::extractRawText(json), QStringLiteral("hello"));

        QJsonObject json2;
        json2["rawText"] = "world";
        QCOMPARE(OcrParser::extractRawText(json2), QStringLiteral("world"));
    }

    void testOcrParser_tryParseJson_markdownBlock()
    {
        QString md = "```json\n{\"a\":1}\n```";
        QJsonDocument doc = OcrParser::tryParseJson(md);
        QVERIFY(!doc.isNull());
        QVERIFY(doc.isObject());
        QCOMPARE(doc.object().value("a").toInt(), 1);
    }

    void testOcrParser_extractLabeledAmount_basic()
    {
        QString text = "价税合计（小写）¥1234.56\n税额 ¥34.56";
        double total = OcrParser::extractLabeledAmount(text,
            {QStringLiteral("价税合计"), QStringLiteral("合计")});
        QVERIFY(total > 1234.0 && total < 1235.0);
    }

    void testParseRaw_invoiceNumber()
    {
        QString text = "发票号码: 12345678\n价税合计 ¥100.00";
        InvoiceData inv = parseRawText(text);
        QCOMPARE(inv.invoiceNumber, QStringLiteral("12345678"));
    }

    void testParseRaw_invoiceNumber_tooShort()
    {
        QString text = "发票号: 123\n价税合计 ¥100.00";
        InvoiceData inv = parseRawText(text);
        QVERIFY(inv.invoiceNumber.length() < 8 || !inv.isValidInvoice);
    }

    void testParseRaw_date()
    {
        QString text = "开票日期: 2024年03月15日\n价税合计 ¥100.00\n发票号码 12345678";
        InvoiceData inv = parseRawText(text);
        QCOMPARE(inv.invoiceDate.year(), 2024);
        QCOMPARE(inv.invoiceDate.month(), 3);
        QCOMPARE(inv.invoiceDate.day(), 15);
    }

    void testParseRaw_totalAmount()
    {
        QString text = "发票号码 12345678\n价税合计（小写）¥1234.56";
        InvoiceData inv = parseRawText(text);
        QVERIFY(qAbs(inv.totalAmount - 1234.56) < 0.01);
    }

    void testParseRaw_taxRate_percent()
    {
        QString text = "发票号码 12345678\n税率 9%\n价税合计 ¥100.00";
        InvoiceData inv = parseRawText(text);
        QVERIFY(qAbs(inv.taxRate - 9.0) < 0.01);
    }

    void testParseRaw_nonInvoiceKeyword()
    {
        QString text = "登机牌\n航班 CA1234\n发票号码 12345678";
        InvoiceData inv = parseRawText(text);
        QVERIFY(!inv.isValidInvoice);
    }

    void testParseStructured_basicFields()
    {
        QJsonObject obj;
        obj["invoiceNumber"] = QStringLiteral("98765432");
        obj["invoiceType"] = QStringLiteral("增值税普通发票");
        obj["totalAmount"] = 999.00;
        obj["sellerName"] = QStringLiteral("测试公司");
        InvoiceData inv = parseStructured(obj);
        QCOMPARE(inv.invoiceNumber, QStringLiteral("98765432"));
        QCOMPARE(inv.invoiceType, QStringLiteral("增值税普通发票"));
        QVERIFY(qAbs(inv.totalAmount - 999.0) < 0.01);
        QCOMPARE(inv.sellerName, QStringLiteral("测试公司"));
    }
};

QTEST_GUILESS_MAIN(TestInvoiceRecognizer)
#include "test_invoice_recognizer.moc"
