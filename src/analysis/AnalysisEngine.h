#pragma once
#include <QString>
#include <QVector>
#include <cmath>
#include "../data/DataSet.h"

namespace StatPro {

struct DescriptiveRow {
    QString variable, label;
    int observations{0};
    int valid{0};
    int blank{0};
    int declaredMissing{0};
    int nonNumeric{0};
    double mean{NAN}, stdDev{NAN}, variance{NAN}, minimum{NAN}, q1{NAN}, median{NAN}, q3{NAN}, maximum{NAN}, skewness{NAN}, kurtosis{NAN};
};

struct FrequencyRow {
    QString value;
    int count{0};
    double percent{0.0};
    double validPercent{0.0};
    double cumulativeValidPercent{0.0};
    bool special{false};
};

struct FrequencySummary {
    int observations{0};
    int valid{0};
    int blank{0};
    int declaredMissing{0};
    int nonNumeric{0};
};

struct GroupSummaryRow {
    QString group;
    int observations{0};
    int valid{0};
    int blank{0};
    int declaredMissing{0};
    int nonNumeric{0};
    double mean{NAN}, stdDev{NAN}, minimum{NAN}, median{NAN}, maximum{NAN};
};

class AnalysisEngine {
public:
    static QVector<DescriptiveRow> descriptive(const DataSet&, const QVector<int>& columns, const QVector<int>& rows = {});
    static QVector<FrequencyRow> frequencies(const DataSet&, int column, const QVector<int>& rows = {});
    static FrequencySummary frequencySummary(const DataSet&, int column, const QVector<int>& rows = {});
    static QVector<GroupSummaryRow> summaryByGroup(const DataSet&, int groupColumn, int valueColumn, const QVector<int>& rows = {});
    static QString number(double value);

private:
    static bool numericValue(const DataSet&, int row, int column, double& out);
    static QString classify(const DataSet&, int row, int column);
    static int countClass(const DataSet&, int column, const QVector<int>& rows, const QString& classification);
    static double percentile(QVector<double> values, double p);
};
}
