#include "TableExtractor.h"
#include "../core/OcrManager.h"
#include <QtConcurrent>

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
                this, &TableExtractor::extractionError);
    }
}

void TableExtractor::extract(const QImage &image)
{
    if (!m_ocrManager) {
        emit extractionError(tr("OCR管理器未设置"));
        return;
    }
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
    TableData table = parseTableData(result);
    emit extractionFinished(table);
}

TableData TableExtractor::parseTableData(const QJsonObject &json)
{
    TableData table;

    // 表格标题
    table.title = json["tableName"].toString();

    // 表头
    if (json.contains("headers") && json["headers"].isArray()) {
        QJsonArray headers = json["headers"].toArray();
        for (const auto &header : headers) {
            table.headers.append(header.toString());
        }
    }

    // 数据行
    if (json.contains("rows") && json["rows"].isArray()) {
        QJsonArray rows = json["rows"].toArray();
        for (const auto &row : rows) {
            QList<TableCell> rowList;
            if (row.isArray()) {
                QJsonArray rowArray = row.toArray();
                for (const auto &cell : rowArray) {
                    TableCell tableCell;
                    tableCell.text = cell.toString();
                    rowList.append(tableCell);
                }
            }
            table.rows.append(rowList);
        }
    }

    return table;
}
