#pragma once
#include <QString>
#include <QVector>
#include <cmath>
#include "../data/DataSet.h"

namespace StatPro {
struct DescriptiveRow {
    QString variable, label;
    int n{0}, missing{0};
    double mean{NAN}, stdDev{NAN}, variance{NAN}, minimum{NAN}, q1{NAN}, median{NAN}, q3{NAN}, maximum{NAN}, skewness{NAN}, kurtosis{NAN};
};
struct FrequencyRow {
    QString value;
    int count{0};
    double percent{0.0};
    double cumulativePercent{0.0};
};

struct GroupSummaryRow {
    QString group;
    int n{0};
    int missing{0};
    double mean{NAN}, stdDev{NAN}, minimum{NAN}, median{NAN}, maximum{NAN};
};

class AnalysisEngine {
public:
    static QVector<DescriptiveRow> descriptive(const DataSet&, const QVector<int>& columns, const QVector<int>& rows = {});
    static QVector<FrequencyRow> frequencies(const DataSet&, int column, const QVector<int>& rows = {});
    static QString formatFrequencyTable(const DataSet&, int column, const QVector<FrequencyRow>&);
    static QVector<GroupSummaryRow> summaryByGroup(const DataSet&, int groupColumn, int valueColumn, const QVector<int>& rows = {});
    static QString formatGroupSummaryTable(const DataSet&, int groupColumn, int valueColumn, const QVector<GroupSummaryRow>&);
    static QString formatDescriptiveTable(const QVector<DescriptiveRow>&, const QString& title = "Descriptive Statistics");
private:
    static bool numericValue(const DataSet&, int row, int column, double& out);
    static double percentile(QVector<double> values, double p);
    static QString number(double value);
};
}
