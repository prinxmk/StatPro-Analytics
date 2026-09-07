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
    QString group1, group2;
    QStringList availableGroups; int n1{0}, n2{0}; double mean1{NAN}, mean2{NAN}, sd1{NAN}, sd2{NAN}, difference{NAN}, t{NAN}, df{NAN}, p{NAN}, ciLow{NAN}, ciHigh{NAN}, cohensD{NAN};
};
struct PairedTResult : ObservationAccounting { int pairs{0}; double meanDifference{NAN}, sdDifference{NAN}, t{NAN}, df{NAN}, p{NAN}, ciLow{NAN}, ciHigh{NAN}, cohensDz{NAN}; };
struct ChiSquareResult : ObservationAccounting { int rows{0}, columns{0}; double chiSquare{NAN}, df{NAN}, p{NAN}, cramersV{NAN}; QVector<QString> rowLabels, columnLabels; QVector<QVector<double>> observed, expected; };
struct NonparametricResult : ObservationAccounting {
    QString group1, group2; int n1{0}, n2{0}; int pairs{0}; int groups{0};
    double statistic{NAN}, z{NAN}, p{NAN}, effectSize{NAN};
    double meanRank1{NAN}, meanRank2{NAN}, u1{NAN}, u2{NAN};
    double wPlus{NAN}, wMinus{NAN};
    QStringList groupLabels; QVector<int> groupNs; QVector<double> groupMeanRanks;
};
struct SpearmanResult : ObservationAccounting { int pairs{0}; double rho{NAN}, t{NAN}, df{NAN}, p{NAN}; };
struct AnovaGroup { QString group; int observations{0}, valid{0}, blank{0}, declaredMissing{0}, nonNumeric{0}; double mean{NAN}, stdDev{NAN}; };
struct AnovaResult : ObservationAccounting { int groups{0}; double grandMean{NAN}, ssBetween{NAN}, ssWithin{NAN}, ssTotal{NAN}, msBetween{NAN}, msWithin{NAN}, f{NAN}, dfBetween{NAN}, dfWithin{NAN}, p{NAN}, etaSquared{NAN}; QVector<AnovaGroup> groupStats; };
struct RegressionResult {
    int observations{0}, complete{0}, xBlank{0}, yBlank{0}, xDeclaredMissing{0}, yDeclaredMissing{0}, xNonNumeric{0}, yNonNumeric{0};
    double intercept{NAN}, slope{NAN}, seIntercept{NAN}, seSlope{NAN}, tIntercept{NAN}, tSlope{NAN}, pIntercept{NAN}, pSlope{NAN};
    double interceptCiLow{NAN}, interceptCiHigh{NAN}, slopeCiLow{NAN}, slopeCiHigh{NAN};
    double r{NAN}, rSquared{NAN}, adjustedRSquared{NAN}, ssRegression{NAN}, ssResidual{NAN}, ssTotal{NAN};
    double msRegression{NAN}, msResidual{NAN}, f{NAN}, fP{NAN}, dfRegression{NAN}, dfResidual{NAN}, rmse{NAN}, durbinWatson{NAN};
};

struct MultipleRegressionCoefficient {
    QString term;
    double estimate{NAN}, stdError{NAN}, standardizedBeta{NAN}, t{NAN}, p{NAN}, ciLow{NAN}, ciHigh{NAN}, vif{NAN};
};
struct MultipleRegressionResult {
    int observations{0}, complete{0}, excludedBlank{0}, excludedDeclaredMissing{0}, excludedNonNumeric{0};
    int predictors{0};
    bool singular{false};
    double rSquared{NAN}, adjustedRSquared{NAN}, ssRegression{NAN}, ssResidual{NAN}, ssTotal{NAN};
    double msRegression{NAN}, msResidual{NAN}, f{NAN}, fP{NAN}, dfRegression{NAN}, dfResidual{NAN}, rmse{NAN}, durbinWatson{NAN};
    QVector<MultipleRegressionCoefficient> coefficients;
};


struct RegressionDiagnosticRow {
    int observation{0};
    double actual{NAN}, predicted{NAN}, residual{NAN}, standardizedResidual{NAN}, studentizedResidual{NAN};
    double leverage{NAN}, cooksDistance{NAN};
    bool highLeverage{false}, influential{false}, largeResidual{false};
};
struct RegressionPredictionRow {
    int observation{0}; double x{NAN}, actual{NAN}, fitted{NAN}, residual{NAN};
    double meanCiLow{NAN}, meanCiHigh{NAN}, predictionLow{NAN}, predictionHigh{NAN};
};
struct RegressionPredictionResult {
    int observations{0}, complete{0}; int movingWindow{3}; double intercept{NAN}, slope{NAN}, rmse{NAN}, meanX{NAN}, sxx{NAN};
    double rSquared{NAN}, dfResidual{NAN}; QVector<RegressionPredictionRow> rows;
};

struct LogisticCoefficient {
    QString term; double estimate{NAN}, stdError{NAN}, z{NAN}, p{NAN}, oddsRatio{NAN}, ciLow{NAN}, ciHigh{NAN};
};
struct LogisticRegressionResult {
    int observations{0}, complete{0}, excludedBlank{0}, excludedDeclaredMissing{0}, excludedNonNumeric{0};
    int predictors{0}, parameters{0}, iterations{0}, outcomeLevels{0}; QString outcomeLevel0, outcomeLevel1; bool converged{false}, singular{false};
    double logLikelihood{NAN}, nullLogLikelihood{NAN}, minus2LogLikelihood{NAN}, aic{NAN}, bic{NAN};
    double mcfaddenR2{NAN}, accuracy{NAN}, sensitivity{NAN}, specificity{NAN};
    int truePositive{0}, trueNegative{0}, falsePositive{0}, falseNegative{0};
    QVector<LogisticCoefficient> coefficients;
};

struct TimeSeriesRow {
    int observation{0}; QString time; double value{NAN}, lag1{NAN}, difference{NAN}, percentChange{NAN}, movingAverage{NAN};
};
struct TimeSeriesResult {
    int observations{0}, valid{0}, excludedBlank{0}, excludedDeclaredMissing{0}, excludedNonNumeric{0};
    int movingWindow{3}; double mean{NAN}, stdDev{NAN}, min{NAN}, max{NAN}, trendSlope{NAN}, trendR2{NAN};
    double firstDifferenceMean{NAN}, firstDifferenceSd{NAN}, acf1{NAN};
    QVector<TimeSeriesRow> rows;
};

struct EconometricCoefficient {
    QString term; double estimate{NAN}, robustStdError{NAN}, zOrT{NAN}, pValue{NAN}, ciLow{NAN}, ciHigh{NAN};
};
struct EconometricResult {
    int observations{0}, complete{0}, predictors{0}, parameters{0};
    int excludedBlank{0}, excludedDeclaredMissing{0}, excludedNonNumeric{0};
    bool singular{false}; double rSquared{NAN}, adjustedRSquared{NAN}, rmse{NAN};
    double f{NAN}, fP{NAN}, durbinWatson{NAN};
    double breuschPaganLM{NAN}, breuschPaganP{NAN}; QVector<EconometricCoefficient> coefficients;
};
struct ADFResult {
    int observations{0}, valid{0}; double statistic{NAN}; QString conclusion; double critical1{NAN}, critical5{NAN}, critical10{NAN};
};

struct RegressionDiagnosticsResult {
    int observations{0}, complete{0}, excludedBlank{0}, excludedDeclaredMissing{0}, excludedNonNumeric{0};
    int predictors{0}, parameters{0};
    bool singular{false};
    double mse{NAN}, rmse{NAN}, rSquared{NAN}, adjustedRSquared{NAN};
    double durbinWatson{NAN}, maxLeverage{NAN}, maxCooksDistance{NAN};
    int highLeverageCount{0}, influentialCount{0}, largeResidualCount{0};
    double jarqueBera{NAN}, jarqueBeraP{NAN};
    QVector<RegressionDiagnosticRow> rows;
};

class AnalysisEngine {
public:
    static QVector<DescriptiveRow> descriptive(const DataSet&, const QVector<int>& columns, const QVector<int>& rows = {});
    static QVector<FrequencyRow> frequencies(const DataSet&, int column, const QVector<int>& rows = {});
    static FrequencySummary frequencySummary(const DataSet&, int column, const QVector<int>& rows = {});
    static QVector<GroupSummaryRow> summaryByGroup(const DataSet&, int groupColumn, int valueColumn, const QVector<int>& rows = {});

    static CorrelationResult pearsonCorrelation(const DataSet&, int xColumn, int yColumn, const QVector<int>& rows = {});
    static OneSampleTResult oneSampleTTest(const DataSet&, int column, double testMean, const QVector<int>& rows = {});
    static IndependentTResult independentTTest(const DataSet&, int groupColumn, int valueColumn, const QString& group1, const QString& group2, bool equalVariances = false, const QVector<int>& rows = {});
    static QStringList independentGroupLevels(const DataSet&, int groupColumn, const QVector<int>& rows = {});
    static PairedTResult pairedTTest(const DataSet&, int firstColumn, int secondColumn, const QVector<int>& rows = {});
    static ChiSquareResult chiSquare(const DataSet&, int rowColumn, int columnColumn, const QVector<int>& rows = {});
    static AnovaResult oneWayAnova(const DataSet&, int groupColumn, int valueColumn, const QVector<int>& rows = {});
    static NonparametricResult mannWhitneyU(const DataSet&, int groupColumn, int valueColumn, const QString& group1, const QString& group2, const QVector<int>& rows = {});
    static NonparametricResult wilcoxonSignedRank(const DataSet&, int firstColumn, int secondColumn, const QVector<int>& rows = {});
    static NonparametricResult kruskalWallis(const DataSet&, int groupColumn, int valueColumn, const QVector<int>& rows = {});
    static SpearmanResult spearmanCorrelation(const DataSet&, int xColumn, int yColumn, const QVector<int>& rows = {});
    static RegressionResult simpleLinearRegression(const DataSet&, int xColumn, int yColumn, const QVector<int>& rows = {});
    static MultipleRegressionResult multipleLinearRegression(const DataSet&, const QVector<int>& predictorColumns, int yColumn, const QVector<int>& rows = {});
    static MultipleRegressionResult regressionWithCategoricalPredictors(const DataSet&, const QVector<int>& predictorColumns, int yColumn, const QVector<int>& rows = {});
    static RegressionPredictionResult regressionPrediction(const DataSet&, int xColumn, int yColumn, int movingWindow = 3, const QVector<int>& rows = {});
    static LogisticRegressionResult logisticRegression(const DataSet&, int yColumn, const QVector<int>& predictorColumns, const QVector<int>& rows = {});
    static TimeSeriesResult timeSeriesAnalysis(const DataSet&, int timeColumn, int valueColumn, int movingWindow = 3, const QVector<int>& rows = {});
    static EconometricResult econometricRobustOls(const DataSet&, int yColumn, const QVector<int>& predictorColumns, const QVector<int>& rows = {});
    static ADFResult augmentedDickeyFuller(const DataSet&, int valueColumn, const QVector<int>& rows = {});
    static RegressionDiagnosticsResult regressionDiagnostics(const DataSet&, const QVector<int>& predictorColumns, int yColumn, const QVector<int>& rows = {});

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
