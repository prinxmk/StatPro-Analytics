#include "AnalysisEngine.h"
#include <algorithm>
#include <QTextStream>

namespace StatPro {

bool AnalysisEngine::numericValue(const DataSet& data, int row, int column, double& out) {
    if (row < 0 || row >= data.rowCount() || column < 0 || column >= data.columnCount() || data.isMissing(row, column)) return false;
    bool ok=false;
    out=data.value(row,column).toString().trimmed().toDouble(&ok);
    return ok && std::isfinite(out);
}

double AnalysisEngine::percentile(QVector<double> values, double p) {
    if(values.isEmpty()) return NAN;
    std::sort(values.begin(), values.end());
    if(values.size()==1) return values.first();
    const double pos=(values.size()-1)*p;
    const int lo=static_cast<int>(std::floor(pos));
    const int hi=static_cast<int>(std::ceil(pos));
    if(lo==hi) return values[lo];
    return values[lo]+(values[hi]-values[lo])*(pos-lo);
}

QVector<DescriptiveRow> AnalysisEngine::descriptive(const DataSet& data, const QVector<int>& columns, const QVector<int>& rows) {
    QVector<DescriptiveRow> result;
    QVector<int> useRows=rows;
    if(useRows.isEmpty()) { useRows.reserve(data.rowCount()); for(int r=0;r<data.rowCount();++r) useRows.push_back(r); }
    for(int c:columns) {
        if(c<0 || c>=data.columnCount()) continue;
        const auto& v=data.variables()[c];
        if(v.type!=VariableType::Numeric) continue;
        DescriptiveRow s; s.variable=v.name; s.label=v.label;
        QVector<double> x; x.reserve(useRows.size());
        for(int r:useRows) { double value; if(numericValue(data,r,c,value)) x.push_back(value); else ++s.missing; }
        s.n=x.size();
        if(x.isEmpty()) { result.push_back(s); continue; }
        double sum=0; for(double value:x) sum+=value; s.mean=sum/x.size();
        s.minimum=*std::min_element(x.begin(),x.end()); s.maximum=*std::max_element(x.begin(),x.end());
        s.q1=percentile(x,.25); s.median=percentile(x,.5); s.q3=percentile(x,.75);
        if(x.size()>1) {
            double ss=0; for(double value:x){const double d=value-s.mean;ss+=d*d;} s.variance=ss/(x.size()-1); s.stdDev=std::sqrt(s.variance);
            if(s.stdDev>0 && x.size()>2) { double m3=0; for(double value:x)m3+=std::pow((value-s.mean)/s.stdDev,3); s.skewness=(static_cast<double>(x.size())/((x.size()-1)*(x.size()-2)))*m3; }
            if(s.stdDev>0 && x.size()>3) { double m4=0; for(double value:x)m4+=std::pow((value-s.mean)/s.stdDev,4); s.kurtosis=((static_cast<double>(x.size())*(x.size()+1))/((x.size()-1)*(x.size()-2)*(x.size()-3)))*m4-(3.0*(x.size()-1)*(x.size()-1))/((x.size()-2)*(x.size()-3)); }
        }
        result.push_back(s);
    }
    return result;
}

QString AnalysisEngine::number(double value) { return std::isfinite(value) ? QString::number(value,'f',4) : "—"; }

QString AnalysisEngine::formatDescriptiveTable(const QVector<DescriptiveRow>& rows, const QString& title) {
    QString out; QTextStream ts(&out);
    auto cell=[](const QString& value, int width) { return value.rightJustified(width, QLatin1Char(' ')); };
    auto left=[](const QString& value, int width) { return value.leftJustified(width, QLatin1Char(' ')); };
    ts << title << "\n";
    ts << QString(142, '=') << "\n";
    ts << left("Variable",18) << cell("N",8) << cell("Missing",9) << cell("Mean",12)
       << cell("Std. Dev.",12) << cell("Variance",12) << cell("Min",12) << cell("Q1",12)
       << cell("Median",12) << cell("Q3",12) << cell("Max",12) << cell("Skewness",12) << cell("Kurtosis",12) << "\n";
    ts << QString(142, '-') << "\n";
    for(const auto& r:rows) {
        ts << left(r.variable.left(18),18) << cell(QString::number(r.n),8) << cell(QString::number(r.missing),9)
           << cell(number(r.mean),12) << cell(number(r.stdDev),12) << cell(number(r.variance),12)
           << cell(number(r.minimum),12) << cell(number(r.q1),12) << cell(number(r.median),12)
           << cell(number(r.q3),12) << cell(number(r.maximum),12) << cell(number(r.skewness),12)
           << cell(number(r.kurtosis),12) << "\n";
    }
    ts << "\nNotes: N excludes missing/non-numeric values. Quartiles use linear interpolation. Kurtosis is excess kurtosis.\n";
    return out;
}
}
