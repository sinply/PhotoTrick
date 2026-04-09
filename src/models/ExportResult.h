#ifndef EXPORTRESULT_H
#define EXPORTRESULT_H

#include <QString>
#include <QList>
#include "InvoiceData.h"
#include "ItineraryData.h"
#include "TableData.h"

struct ExportResult {
    // Processing results
    QList<InvoiceData> invoices;
    QList<ItineraryData> itineraries;
    QList<TableData> tables;

    // Summary
    double totalAmount = 0.0;
    double totalTax = 0.0;

    // Statistics by category
    int transportationCount = 0;
    int accommodationCount = 0;
    int diningCount = 0;

    // Errors
    QStringList errors;

    // Processing time
    qint64 processingTimeMs = 0;

    bool isEmpty() const {
        return invoices.isEmpty() && itineraries.isEmpty() && tables.isEmpty();
    }

    void calculateSummary();

    // Serialization
    QJsonObject toJson() const;
    static ExportResult fromJson(const QJsonObject &json);
};

#endif // EXPORTRESULT_H
