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
class AnalysisEngine {
public:
    static QVector<DescriptiveRow> descriptive(const DataSet&, const QVector<int>& columns, const QVector<int>& rows = {});
    static QString formatDescriptiveTable(const QVector<DescriptiveRow>&, const QString& title = "Descriptive Statistics");
private:
    static bool numericValue(const DataSet&, int row, int column, double& out);
    static double percentile(QVector<double> values, double p);
    static QString number(double value);
};
}
