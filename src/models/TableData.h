#ifndef TABLEDATA_H
#define TABLEDATA_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QJsonObject>
#include <QVariant>

struct TableCell {
    QString text;
    int rowSpan = 1;
    int colSpan = 1;

    QJsonObject toJson() const;
    static TableCell fromJson(const QJsonObject &json);
};

struct TableData {
    QStringList headers;
    QList<QList<TableCell>> rows;
    QString title;

    int rowCount() const { return rows.size(); }
    int columnCount() const { return headers.size(); }

    bool isEmpty() const { return rows.isEmpty(); }

    QString cellText(int row, int col) const;
    void setCellText(int row, int col, const QString &text);

    // Serialization
    QJsonObject toJson() const;
    static TableData fromJson(const QJsonObject &json);

    // Convert to CSV
    QString toCsv() const;

    // Convert to Markdown
    QString toMarkdown() const;

private:
    QString escapeCsvField(const QString &field) const;
};

#endif // TABLEDATA_H
