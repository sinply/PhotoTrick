#include "TableData.h"
#include <QJsonArray>

QJsonObject TableCell::toJson() const
{
    QJsonObject obj;
    obj["text"] = text;
    obj["rowSpan"] = rowSpan;
    obj["colSpan"] = colSpan;
    return obj;
}

TableCell TableCell::fromJson(const QJsonObject &json)
{
    TableCell cell;
    cell.text = json["text"].toString();
    cell.rowSpan = json["rowSpan"].toInt(1);
    cell.colSpan = json["colSpan"].toInt(1);
    return cell;
}

QString TableData::cellText(int row, int col) const
{
    if (row < 0 || row >= rows.size()) return QString();
    if (col < 0 || col >= rows[row].size()) return QString();
    return rows[row][col].text;
}

void TableData::setCellText(int row, int col, const QString &text)
{
    if (row < 0 || col < 0) return;

    // Expand rows if needed
    while (rows.size() <= row) {
        rows.append(QList<TableCell>());
    }

    // Expand columns if needed
    while (rows[row].size() <= col) {
        rows[row].append(TableCell());
    }

    rows[row][col].text = text;
}

QJsonObject TableData::toJson() const
{
    QJsonObject obj;

    if (!title.isEmpty()) {
        obj["title"] = title;
    }

    // Headers
    QJsonArray headersArray;
    for (const QString &header : headers) {
        headersArray.append(header);
    }
    obj["headers"] = headersArray;

    // Rows
    QJsonArray rowsArray;
    for (const auto &row : rows) {
        QJsonArray rowArray;
        for (const auto &cell : row) {
            rowArray.append(cell.toJson());
        }
        rowsArray.append(rowArray);
    }
    obj["rows"] = rowsArray;

    return obj;
}

TableData TableData::fromJson(const QJsonObject &json)
{
    TableData data;
    data.title = json["title"].toString();

    QJsonArray headersArray = json["headers"].toArray();
    for (const auto &header : headersArray) {
        data.headers.append(header.toString());
    }

    QJsonArray rowsArray = json["rows"].toArray();
    for (const auto &row : rowsArray) {
        QList<TableCell> rowData;
        QJsonArray rowArray = row.toArray();
        for (const auto &cell : rowArray) {
            rowData.append(TableCell::fromJson(cell.toObject()));
        }
        data.rows.append(rowData);
    }

    return data;
}

QString TableData::toCsv() const
{
    QString result;

    // Headers
    if (!headers.isEmpty()) {
        QStringList escapedHeaders;
        for (const QString &header : headers) {
            escapedHeaders << escapeCsvField(header);
        }
        result += escapedHeaders.join(",") + "\n";
    }

    // Rows
    for (const auto &row : rows) {
        QStringList escapedRow;
        for (const auto &cell : row) {
            escapedRow << escapeCsvField(cell.text);
        }
        result += escapedRow.join(",") + "\n";
    }

    return result;
}

QString TableData::toMarkdown() const
{
    QString result;

    // Title
    if (!title.isEmpty()) {
        result += QString("# %1\n\n").arg(title);
    }

    // Headers
    if (!headers.isEmpty()) {
        result += "|";
        for (const QString &header : headers) {
            result += QString(" %1 |").arg(header);
        }
        result += "\n|";
        for (int i = 0; i < headers.size(); ++i) {
            result += "------|";
        }
        result += "\n";
    }

    // Rows
    for (const auto &row : rows) {
        result += "|";
        for (const auto &cell : row) {
            result += QString(" %1 |").arg(cell.text);
        }
        result += "\n";
    }

    return result;
}

QString TableData::escapeCsvField(const QString &field) const
{
    if (field.contains(',') || field.contains('"') || field.contains('\n')) {
        QString escaped = field;
        escaped.replace("\"", "\"\"");
        return "\"" + escaped + "\"";
    }
    return field;
}
