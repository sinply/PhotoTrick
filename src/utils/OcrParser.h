#ifndef OCRPARSER_H
#define OCRPARSER_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonDocument>
#include <QList>

namespace OcrParser {

// Parse JSON object from text that may be wrapped in markdown code blocks
QJsonObject parseJsonObjectFromText(QString text);

// Parse JSON document (object or array) from text
QJsonDocument tryParseJson(QString text);

// Normalize a key for fuzzy matching (lowercase, remove separators)
QString normalizeKey(QString key);

// Check if a string contains meaningful data (not null/none/dash)
bool isMeaningfulString(const QString &value);

// Find first non-empty value from a JSON object by key list
QString firstNonEmpty(const QJsonObject &json, const QStringList &keys);

// Deep search for a value by normalized key names
QJsonValue findValueByKeysDeep(const QJsonValue &root, const QStringList &keys);

// Extract raw text from OCR result JSON (handles multiple key formats)
QString extractRawText(const QJsonObject &json);

// Choose the primary data object from a wrapped JSON response
QJsonObject choosePrimaryObject(const QJsonObject &json);

// Parse a numeric value from JSON (handles strings with currency symbols)
double parseNumber(const QJsonValue &value);

// Extract an amount from raw text using label-based scoring
// positiveLabels: labels that indicate the target amount
// negativeLabels: labels that indicate non-amount numbers (optional)
double extractLabeledAmount(const QString &rawText,
                            const QStringList &positiveLabels,
                            const QStringList &negativeLabels = {});

} // namespace OcrParser

#endif // OCRPARSER_H
