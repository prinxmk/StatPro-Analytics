#include "AnalysisEngine.h"
#include <algorithm>
#include <QTextStream>
#include <QMap>

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


QVector<FrequencyRow> AnalysisEngine::frequencies(const DataSet& data, int column, const QVector<int>& rows) {
    QVector<FrequencyRow> result;
    if(column<0 || column>=data.columnCount()) return result;
    QVector<int> useRows=rows;
    if(useRows.isEmpty()){ useRows.reserve(data.rowCount()); for(int r=0;r<data.rowCount();++r) useRows.push_back(r); }
    QMap<QString,int> counts;
    int valid=0;
    for(int r:useRows){
        if(r<0 || r>=data.rowCount() || data.isMissing(r,column)) continue;
        QString value=data.value(r,column).toString().trimmed();
        if(value.isEmpty()) continue;
        ++counts[value]; ++valid;
    }
    if(valid==0) return result;
    int cumulative=0;
    for(auto it=counts.cbegin();it!=counts.cend();++it){
        FrequencyRow f; f.value=it.key(); f.count=it.value(); cumulative+=f.count;
        f.percent=100.0*f.count/valid; f.cumulativePercent=100.0*cumulative/valid; result.push_back(f);
    }
    return result;
}

QString AnalysisEngine::formatFrequencyTable(const DataSet& data, int column, const QVector<FrequencyRow>& rows) {
    QString out; QTextStream ts(&out);
    const QString name=(column>=0 && column<data.columnCount())?data.variables()[column].name:"Variable";
    int total=0; for(const auto& r:rows) total+=r.count;
    ts << "Frequencies: " << name << "\n" << QString(68,'=') << "\n";
    ts << QString("%1%2%3%4\n").arg("Value",30).arg("Frequency",12).arg("Percent",12).arg("Cum. Percent",14);
    ts << QString(68,'-') << "\n";
    for(const auto& r:rows)
        ts << r.value.left(30).leftJustified(30) << QString::number(r.count).rightJustified(12)
           << QString::number(r.percent,'f',2).rightJustified(12) << QString::number(r.cumulativePercent,'f',2).rightJustified(14) << "\n";
    ts << QString(68,'-') << "\n" << "Valid total" << QString::number(total).rightJustified(54) << "\n";
    ts << "Note: blank and declared-missing values are excluded from percentages.\n";
    return out;
}

QVector<GroupSummaryRow> AnalysisEngine::summaryByGroup(const DataSet& data, int groupColumn, int valueColumn, const QVector<int>& rows) {
    QVector<GroupSummaryRow> result;
    if(groupColumn<0 || groupColumn>=data.columnCount() || valueColumn<0 || valueColumn>=data.columnCount() ||
       data.variables()[valueColumn].type!=VariableType::Numeric) return result;
    QVector<int> useRows=rows;
    if(useRows.isEmpty()){ useRows.reserve(data.rowCount()); for(int r=0;r<data.rowCount();++r) useRows.push_back(r); }
    QMap<QString,QVector<double>> groups;
    QMap<QString,int> missing;
    for(int r:useRows){
        if(r<0 || r>=data.rowCount()) continue;
        QString group=data.isMissing(r,groupColumn)?"(Missing group)":data.value(r,groupColumn).toString().trimmed();
        if(group.isEmpty()) group="(Blank)";
        double value;
        if(numericValue(data,r,valueColumn,value)) groups[group].push_back(value); else ++missing[group];
    }
    for(auto it=groups.cbegin();it!=groups.cend();++it){
        const auto& x=it.value(); GroupSummaryRow g; g.group=it.key(); g.n=x.size(); g.missing=missing.value(it.key());
        if(x.isEmpty()){ result.push_back(g); continue; }
        double sum=0; for(double v:x) sum+=v; g.mean=sum/x.size();
        QVector<double> sorted=x; std::sort(sorted.begin(),sorted.end()); g.minimum=sorted.first(); g.maximum=sorted.last(); g.median=percentile(sorted,.5);
        if(x.size()>1){ double ss=0; for(double v:x){double d=v-g.mean;ss+=d*d;} g.stdDev=std::sqrt(ss/(x.size()-1)); }
        result.push_back(g);
    }
    return result;
}

QString AnalysisEngine::formatGroupSummaryTable(const DataSet& data, int groupColumn, int valueColumn, const QVector<GroupSummaryRow>& rows) {
    QString out; QTextStream ts(&out);
    QString group=(groupColumn>=0&&groupColumn<data.columnCount())?data.variables()[groupColumn].name:"Group";
    QString value=(valueColumn>=0&&valueColumn<data.columnCount())?data.variables()[valueColumn].name:"Value";
    ts << "Summary by Group: " << value << " by " << group << "\n" << QString(94,'=') << "\n";
    ts << QString("%1%2%3%4%5%6%7\n").arg("Group",24).arg("N",8).arg("Missing",10).arg("Mean",13).arg("Std. Dev.",13).arg("Median",13).arg("Min / Max",18);
    ts << QString(94,'-') << "\n";
    for(const auto& r:rows){
        QString mm=number(r.minimum)+" / "+number(r.maximum);
        ts << r.group.left(24).leftJustified(24) << QString::number(r.n).rightJustified(8) << QString::number(r.missing).rightJustified(10)
           << number(r.mean).rightJustified(13) << number(r.stdDev).rightJustified(13) << number(r.median).rightJustified(13) << mm.rightJustified(18) << "\n";
    }
    ts << "\nNote: N excludes missing/non-numeric outcome values within each group.\n";
    return out;
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
