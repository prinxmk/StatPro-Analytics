#pragma once
#include <QString>
#include <QVector>
#include <cmath>
#include "../data/DataSet.h"

namespace StatPro {

struct DescriptiveRow {
    QString variable, label;
    int observations{0}; int valid{0}; int blank{0}; int declaredMissing{0}; int nonNumeric{0};
    double mean{NAN}, stdDev{NAN}, variance{NAN}, minimum{NAN}, q1{NAN}, median{NAN}, q3{NAN}, maximum{NAN}, skewness{NAN}, kurtosis{NAN};
};
struct FrequencyRow { QString value; int count{0}; double percent{0.0}; double validPercent{0.0}; double cumulativeValidPercent{0.0}; bool special{false}; };
struct FrequencySummary { int observations{0}; int valid{0}; int blank{0}; int declaredMissing{0}; int nonNumeric{0}; };
struct GroupSummaryRow {
    QString group; int observations{0}; int valid{0}; int blank{0}; int declaredMissing{0}; int nonNumeric{0};
    double mean{NAN}, stdDev{NAN}, minimum{NAN}, median{NAN}, maximum{NAN};
};

struct ObservationAccounting { int observations{0}; int valid{0}; int blank{0}; int declaredMissing{0}; int nonNumeric{0}; };
struct CorrelationResult : ObservationAccounting { int pairs{0}; double r{NAN}, p{NAN}, ciLow{NAN}, ciHigh{NAN}; };
struct OneSampleTResult : ObservationAccounting { double testMean{NAN}, mean{NAN}, stdDev{NAN}, t{NAN}, df{NAN}, p{NAN}, ciLow{NAN}, ciHigh{NAN}, cohensD{NAN}; };
struct IndependentTResult {
    ObservationAccounting group1Accounting, group2Accounting;
    QString group1, group2; int n1{0}, n2{0}; double mean1{NAN}, mean2{NAN}, sd1{NAN}, sd2{NAN}, difference{NAN}, t{NAN}, df{NAN}, p{NAN}, ciLow{NAN}, ciHigh{NAN}, cohensD{NAN};
};
struct PairedTResult : ObservationAccounting { int pairs{0}; double meanDifference{NAN}, sdDifference{NAN}, t{NAN}, df{NAN}, p{NAN}, ciLow{NAN}, ciHigh{NAN}, cohensDz{NAN}; };
struct ChiSquareResult : ObservationAccounting { int rows{0}, columns{0}; double chiSquare{NAN}, df{NAN}, p{NAN}, cramersV{NAN}; QVector<QString> rowLabels, columnLabels; QVector<QVector<double>> observed, expected; };
struct AnovaGroup { QString group; int observations{0}, valid{0}, blank{0}, declaredMissing{0}, nonNumeric{0}; double mean{NAN}, stdDev{NAN}; };
struct AnovaResult : ObservationAccounting { int groups{0}; double grandMean{NAN}, ssBetween{NAN}, ssWithin{NAN}, ssTotal{NAN}, msBetween{NAN}, msWithin{NAN}, f{NAN}, dfBetween{NAN}, dfWithin{NAN}, p{NAN}, etaSquared{NAN}; QVector<AnovaGroup> groupStats; };
struct RegressionResult {
    int observations{0}, complete{0}, xBlank{0}, yBlank{0}, xDeclaredMissing{0}, yDeclaredMissing{0}, xNonNumeric{0}, yNonNumeric{0};
    double intercept{NAN}, slope{NAN}, seIntercept{NAN}, seSlope{NAN}, tIntercept{NAN}, tSlope{NAN}, pIntercept{NAN}, pSlope{NAN};
    double interceptCiLow{NAN}, interceptCiHigh{NAN}, slopeCiLow{NAN}, slopeCiHigh{NAN};
    double r{NAN}, rSquared{NAN}, adjustedRSquared{NAN}, ssRegression{NAN}, ssResidual{NAN}, ssTotal{NAN};
    double msRegression{NAN}, msResidual{NAN}, f{NAN}, fP{NAN}, dfRegression{NAN}, dfResidual{NAN}, rmse{NAN}, durbinWatson{NAN};
};

class AnalysisEngine {
public:
    static QVector<DescriptiveRow> descriptive(const DataSet&, const QVector<int>& columns, const QVector<int>& rows = {});
    static QVector<FrequencyRow> frequencies(const DataSet&, int column, const QVector<int>& rows = {});
    static FrequencySummary frequencySummary(const DataSet&, int column, const QVector<int>& rows = {});
    static QVector<GroupSummaryRow> summaryByGroup(const DataSet&, int groupColumn, int valueColumn, const QVector<int>& rows = {});

    static CorrelationResult pearsonCorrelation(const DataSet&, int xColumn, int yColumn, const QVector<int>& rows = {});
    static OneSampleTResult oneSampleTTest(const DataSet&, int column, double testMean, const QVector<int>& rows = {});
    static IndependentTResult independentTTest(const DataSet&, int groupColumn, int valueColumn, bool equalVariances = false, const QVector<int>& rows = {});
    static PairedTResult pairedTTest(const DataSet&, int firstColumn, int secondColumn, const QVector<int>& rows = {});
    static ChiSquareResult chiSquare(const DataSet&, int rowColumn, int columnColumn, const QVector<int>& rows = {});
    static AnovaResult oneWayAnova(const DataSet&, int groupColumn, int valueColumn, const QVector<int>& rows = {});
    static RegressionResult simpleLinearRegression(const DataSet&, int xColumn, int yColumn, const QVector<int>& rows = {});

    static QString number(double value);

private:
    static bool numericValue(const DataSet&, int row, int column, double& out);
    static QString classify(const DataSet&, int row, int column);
    static int countClass(const DataSet&, int column, const QVector<int>& rows, const QString& classification);
    static double percentile(QVector<double> values, double p);
    static double normalCdf(double x);
    static double studentTCdf(double t, double df);
    static double studentTQuantile(double p, double df);
    static double chiSquareSurvival(double x, double df);
    static double fSurvival(double f, double d1, double d2);
    static double logGamma(double x);
    static double regularizedBeta(double x, double a, double b);
    static double regularizedGammaQ(double a, double x);
};
}
