#include "TableExtractor.h"
#include "../core/OcrManager.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QRegularExpression>
#include <QtConcurrent>
#include <algorithm>

namespace {
QJsonDocument parseJsonDocumentFromText(QString text)
{
    text = text.trimmed();
    if (text.startsWith("```")) {
        int firstNewline = text.indexOf('\n');
        if (firstNewline >= 0) {
            text = text.mid(firstNewline + 1);
        }
        if (text.endsWith("```")) {
            text.chop(3);
        }
        text = text.trimmed();
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8(), &err);
    if (!doc.isNull()) {
        return doc;
    }

    const int objStart = text.indexOf('{');
    const int objEnd = text.lastIndexOf('}');
    if (objStart >= 0 && objEnd > objStart) {
        const QString jsonPart = text.mid(objStart, objEnd - objStart + 1);
        doc = QJsonDocument::fromJson(jsonPart.toUtf8(), &err);
        if (!doc.isNull()) {
            return doc;
        }
    }

    const int arrStart = text.indexOf('[');
    const int arrEnd = text.lastIndexOf(']');
    if (arrStart >= 0 && arrEnd > arrStart) {
        const QString jsonPart = text.mid(arrStart, arrEnd - arrStart + 1);
        doc = QJsonDocument::fromJson(jsonPart.toUtf8(), &err);
        if (!doc.isNull()) {
            return doc;
        }
    }

    return QJsonDocument();
}

QString cellToString(const QJsonValue &value)
{
    if (value.isString()) {
        return value.toString().trimmed();
    }
    if (value.isDouble()) {
        return QString::number(value.toDouble(), 'f', 2).remove(QRegularExpression(QStringLiteral("\\.?0+$")));
    }
    if (value.isBool()) {
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    }
    if (value.isObject()) {
        const QJsonObject obj = value.toObject();
        const QStringList textKeys = {"text", "value", "content", "name", "title", "amount"};
        for (const QString &key : textKeys) {
            const QString text = obj.value(key).toString().trimmed();
            if (!text.isEmpty()) {
                return text;
            }
        }
    }
    return QString();
}

QList<TableCell> parseRow(const QJsonValue &rowValue)
{
    QList<TableCell> row;

    if (rowValue.isArray()) {
        const QJsonArray rowArray = rowValue.toArray();
        for (const QJsonValue &cellValue : rowArray) {
            TableCell cell;
            cell.text = cellToString(cellValue);
            row.append(cell);
        }
        return row;
    }

    if (rowValue.isObject()) {
        const QJsonObject rowObj = rowValue.toObject();
        if (rowObj.contains(QStringLiteral("cells")) && rowObj.value(QStringLiteral("cells")).isArray()) {
            return parseRow(rowObj.value(QStringLiteral("cells")));
        }

        QStringList keys = rowObj.keys();
        std::sort(keys.begin(), keys.end());
        for (const QString &key : keys) {
            if (key.startsWith(QStringLiteral("col"), Qt::CaseInsensitive)
                || key.startsWith(QStringLiteral("cell"), Qt::CaseInsensitive)) {
                TableCell cell;
                cell.text = cellToString(rowObj.value(key));
                row.append(cell);
            }
        }
    }

    return row;
}

TableData parseSingleTableObject(const QJsonObject &obj)
{
    TableData table;

    const QStringList titleKeys = {"tableName", "title", "name"};
    for (const QString &k : titleKeys) {
        const QString title = obj.value(k).toString().trimmed();
        if (!title.isEmpty()) {
            table.title = title;
            break;
        }
    }

    const QStringList headerKeys = {"headers", "header", "columns"};
    for (const QString &k : headerKeys) {
        if (obj.contains(k) && obj.value(k).isArray()) {
            const QJsonArray headers = obj.value(k).toArray();
            for (const QJsonValue &header : headers) {
                table.headers.append(cellToString(header));
            }
            if (!table.headers.isEmpty()) {
                break;
            }
        }
    }

    const QStringList rowKeys = {"rows", "data", "body", "tableData"};
    for (const QString &k : rowKeys) {
        if (obj.contains(k) && obj.value(k).isArray()) {
            const QJsonArray rows = obj.value(k).toArray();
            for (const QJsonValue &rowValue : rows) {
                QList<TableCell> row = parseRow(rowValue);
                if (!row.isEmpty()) {
                    table.rows.append(row);
                }
            }
            if (!table.rows.isEmpty()) {
                break;
            }
        }
    }

    if (table.headers.isEmpty() && !table.rows.isEmpty()) {
        QStringList guessedHeaders;
        const int cols = table.rows.first().size();
        for (int i = 0; i < cols; ++i) {
            guessedHeaders.append(QStringLiteral("列%1").arg(i + 1));
        }
        table.headers = guessedHeaders;
    }

    return table;
}

QList<TableData> parseTablesFromValue(const QJsonValue &value)
{
    QList<TableData> tables;

    if (value.isArray()) {
        const QJsonArray arr = value.toArray();
        for (const QJsonValue &item : arr) {
            if (item.isObject()) {
                TableData table = parseSingleTableObject(item.toObject());
                if (!table.rows.isEmpty() || !table.headers.isEmpty()) {
                    tables.append(table);
                }
            }
        }
    } else if (value.isObject()) {
        const QJsonObject obj = value.toObject();
        if (obj.contains(QStringLiteral("tables")) && obj.value(QStringLiteral("tables")).isArray()) {
            tables += parseTablesFromValue(obj.value(QStringLiteral("tables")));
        } else {
            TableData table = parseSingleTableObject(obj);
            if (!table.rows.isEmpty() || !table.headers.isEmpty()) {
                tables.append(table);
            }
        }
    }

    return tables;
}

int tableCellCount(const TableData &table)
{
    int count = 0;
    for (const QList<TableCell> &row : table.rows) {
        count += row.size();
    }
    return count;
}

TableData chooseBestTable(const QList<TableData> &tables)
{
    if (tables.isEmpty()) {
        return TableData();
    }

    TableData best = tables.first();
    int bestScore = tableCellCount(best);
    for (int i = 1; i < tables.size(); ++i) {
        const int score = tableCellCount(tables[i]);
        if (score > bestScore) {
            best = tables[i];
            bestScore = score;
        }
    }
    return best;
}

TableData parsePlainTextTable(const QString &rawText)
{
    TableData table;

    if (rawText.trimmed().isEmpty()) {
        return table;
    }

    const QStringList lines = rawText.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);
    QList<QStringList> rows;

    for (const QString &lineRaw : lines) {
        const QString line = lineRaw.trimmed();
        if (line.isEmpty()) {
            continue;
        }

        QStringList cells;
        if (line.contains('\t')) {
            cells = line.split('\t');
        } else if (line.contains('|')) {
            cells = line.split('|');
            while (!cells.isEmpty() && cells.first().trimmed().isEmpty()) {
                cells.removeFirst();
            }
            while (!cells.isEmpty() && cells.last().trimmed().isEmpty()) {
                cells.removeLast();
            }
        } else {
            cells = line.split(QRegularExpression(QStringLiteral("\\s{2,}")), Qt::SkipEmptyParts);
        }

        for (QString &cell : cells) {
            cell = cell.trimmed();
        }

        if (cells.size() >= 2) {
            rows.append(cells);
        }
    }

    if (rows.size() < 2) {
        return table;
    }

    table.headers = rows.first();
    for (int i = 1; i < rows.size(); ++i) {
        QList<TableCell> row;
        for (const QString &cellText : rows[i]) {
            TableCell cell;
            cell.text = cellText;
            row.append(cell);
        }
        if (!row.isEmpty()) {
            table.rows.append(row);
        }
    }

    return table;
}

QString extractRawText(const QJsonObject &json)
{
    QStringList chunks;
    const QStringList keys = {
        QStringLiteral("text"),
        QStringLiteral("raw_result"),
        QStringLiteral("content")
    };

    for (const QString &key : keys) {
        const QJsonValue val = json.value(key);
        if (val.isString()) {
            const QString text = val.toString().trimmed();
            if (!text.isEmpty()) {
                chunks.append(text);
            }
        }
    }

    if (chunks.isEmpty() && json.contains(QStringLiteral("data")) && json.value(QStringLiteral("data")).isObject()) {
        chunks.append(extractRawText(json.value(QStringLiteral("data")).toObject()));
    }

    return chunks.join('\n').trimmed();
}

QJsonObject choosePrimaryObject(const QJsonObject &json)
{
    if (json.contains(QStringLiteral("data")) && json.value(QStringLiteral("data")).isObject()) {
        return json.value(QStringLiteral("data")).toObject();
    }
    if (json.contains(QStringLiteral("result")) && json.value(QStringLiteral("result")).isObject()) {
        return json.value(QStringLiteral("result")).toObject();
    }
    return json;
}
}

const QString TableExtractor::TABLE_PROMPT = R"(
请分析这张图片中的表格，提取表格内容并以JSON格式返回：

{
    "tableName": "表格名称（如果有标题）",
    "headers": ["列标题1", "列标题2", "列标题3", ...],
    "rows": [
        ["单元格1", "单元格2", "单元格3", ...],
        ["单元格1", "单元格2", "单元格3", ...],
        ...
    ]
}

注意事项：
1. 保持表格的原始行列结构
2. 空单元格用空字符串表示
3. 合并单元格需要标注合并范围
4. 如果图片中有多个表格，请分别返回多个表格对象组成的数组
5. 请确保返回有效的JSON格式
)";

TableExtractor::TableExtractor(QObject *parent)
    : QObject(parent)
    , m_ocrManager(nullptr)
{
}

void TableExtractor::setOcrManager(OcrManager *manager)
{
    if (m_ocrManager) {
        disconnect(m_ocrManager, nullptr, this, nullptr);
    }
    m_ocrManager = manager;
    if (m_ocrManager) {
        connect(m_ocrManager, &OcrManager::recognitionFinished,
                this, &TableExtractor::onOcrFinished);
        connect(m_ocrManager, &OcrManager::recognitionError,
                this, &TableExtractor::onOcrError);
    }
}

void TableExtractor::extract(const QImage &image)
{
    if (!m_ocrManager) {
        emit extractionError(tr("OCR管理器未设置"));
        return;
    }
    m_isExtracting = true;
    m_ocrManager->recognizeImage(image, TABLE_PROMPT);
}

void TableExtractor::extractAsync(const QImage &image)
{
    if (!m_ocrManager) {
        emit extractionError(tr("OCR管理器未设置"));
        return;
    }

    QtConcurrent::run([this, image]() {
        extract(image);
    });
}

void TableExtractor::onOcrFinished(const QJsonObject &result)
{
    if (!m_isExtracting) {
        return;
    }
    m_isExtracting = false;
    emit rawOcrReceived(result);

    TableData table = parseTableData(result);
    emit extractionFinished(table);
}

void TableExtractor::onOcrError(const QString &error)
{
    if (!m_isExtracting) {
        return;
    }
    m_isExtracting = false;
    emit extractionError(error);
}

TableData TableExtractor::parseTableData(const QJsonObject &json)
{
    const QString rawText = extractRawText(json);

    QJsonObject normalized = choosePrimaryObject(json);
    QJsonValue rootValue(normalized);

    if (normalized.contains(QStringLiteral("text")) && normalized.value(QStringLiteral("text")).isString()) {
        const QJsonDocument parsed = parseJsonDocumentFromText(normalized.value(QStringLiteral("text")).toString());
        if (!parsed.isNull()) {
            rootValue = parsed.isArray() ? QJsonValue(parsed.array()) : QJsonValue(parsed.object());
        }
    }

    QList<TableData> parsedTables = parseTablesFromValue(rootValue);
    if (parsedTables.isEmpty() && normalized.contains(QStringLiteral("tables"))) {
        parsedTables = parseTablesFromValue(normalized.value(QStringLiteral("tables")));
    }

    TableData best = chooseBestTable(parsedTables);
    if (!best.rows.isEmpty() || !best.headers.isEmpty()) {
        return best;
    }

    return parsePlainTextTable(rawText);
}
