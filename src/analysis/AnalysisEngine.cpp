#include "AnalysisEngine.h"
#include <algorithm>
#include <QMap>

namespace StatPro {

bool AnalysisEngine::numericValue(const DataSet& data, int row, int column, double& out) {
    if (row < 0 || row >= data.rowCount() || column < 0 || column >= data.columnCount()) return false;
    if (classify(data, row, column) != "Valid") return false;
    bool ok=false;
    out=data.value(row,column).toString().trimmed().toDouble(&ok);
    return ok && std::isfinite(out);
}

QString AnalysisEngine::classify(const DataSet& data, int row, int column) {
    if (row < 0 || row >= data.rowCount() || column < 0 || column >= data.columnCount()) return "Invalid";
    const QString text=data.value(row,column).toString().trimmed();
    if (text.isEmpty()) return "Blank";
    for (const auto& mv : data.variables()[column].missingValues)
        if (text == mv.trimmed()) return "DeclaredMissing";
    if (data.variables()[column].type == VariableType::Numeric) {
        bool ok=false; const double value=text.toDouble(&ok);
        if (!ok || !std::isfinite(value)) return "NonNumeric";
    }
    return "Valid";
}

int AnalysisEngine::countClass(const DataSet& data, int column, const QVector<int>& rows, const QString& classification) {
    int count=0;
    for (int r : rows) if (classify(data,r,column)==classification) ++count;
    return count;
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
        DescriptiveRow s; s.variable=v.name; s.label=v.label; s.observations=useRows.size();
        QVector<double> x; x.reserve(useRows.size());
        for(int r:useRows) {
            const QString cls=classify(data,r,c);
            if(cls=="Valid") { double value; if(numericValue(data,r,c,value)) x.push_back(value); else ++s.nonNumeric; }
            else if(cls=="Blank") ++s.blank;
            else if(cls=="DeclaredMissing") ++s.declaredMissing;
            else if(cls=="NonNumeric") ++s.nonNumeric;
        }
        s.valid=x.size();
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

FrequencySummary AnalysisEngine::frequencySummary(const DataSet& data, int column, const QVector<int>& rows) {
    FrequencySummary summary;
    if(column<0 || column>=data.columnCount()) return summary;
    QVector<int> useRows=rows;
    if(useRows.isEmpty()){ useRows.reserve(data.rowCount()); for(int r=0;r<data.rowCount();++r) useRows.push_back(r); }
    summary.observations=useRows.size();
    for(int r:useRows){
        const QString cls=classify(data,r,column);
        if(cls=="Valid") ++summary.valid;
        else if(cls=="Blank") ++summary.blank;
        else if(cls=="DeclaredMissing") ++summary.declaredMissing;
        else ++summary.nonNumeric;
    }
    return summary;
}

QVector<FrequencyRow> AnalysisEngine::frequencies(const DataSet& data, int column, const QVector<int>& rows) {
    QVector<FrequencyRow> result;
    if(column<0 || column>=data.columnCount()) return result;
    QVector<int> useRows=rows;
    if(useRows.isEmpty()){ useRows.reserve(data.rowCount()); for(int r=0;r<data.rowCount();++r) useRows.push_back(r); }
    QMap<QString,int> counts;
    int valid=0, cumulative=0;
    for(int r:useRows){
        const QString cls=classify(data,r,column);
        QString value;
        if(cls=="Blank") value="(Blank)";
        else if(cls=="DeclaredMissing") value="(Declared missing)";
        else if(cls=="NonNumeric") value="(Non-numeric / invalid)";
        else { value=data.value(r,column).toString().trimmed(); ++valid; }
        ++counts[value];
    }
    for(auto it=counts.cbegin();it!=counts.cend();++it){
        FrequencyRow f; f.value=it.key(); f.count=it.value(); f.special=(it.key().startsWith("("));
        f.percent=useRows.isEmpty()?0.0:100.0*f.count/useRows.size();
        if(!f.special){ cumulative+=f.count; f.validPercent=valid?100.0*f.count/valid:0.0; f.cumulativeValidPercent=valid?100.0*cumulative/valid:0.0; }
        result.push_back(f);
    }
    return result;
}

QVector<GroupSummaryRow> AnalysisEngine::summaryByGroup(const DataSet& data, int groupColumn, int valueColumn, const QVector<int>& rows) {
    QVector<GroupSummaryRow> result;
    if(groupColumn<0 || groupColumn>=data.columnCount() || valueColumn<0 || valueColumn>=data.columnCount() || data.variables()[valueColumn].type!=VariableType::Numeric) return result;
    QVector<int> useRows=rows;
    if(useRows.isEmpty()){ useRows.reserve(data.rowCount()); for(int r=0;r<data.rowCount();++r) useRows.push_back(r); }
    QMap<QString,QVector<double>> groups;
    QMap<QString,GroupSummaryRow> summaries;
    for(int r:useRows){
        QString group;
        const QString groupClass=classify(data,r,groupColumn);
        if(groupClass=="Blank") group="(Blank)";
        else if(groupClass=="DeclaredMissing") group="(Declared missing)";
        else if(groupClass=="NonNumeric") group="(Invalid group)";
        else group=data.value(r,groupColumn).toString().trimmed();
        auto& s=summaries[group]; s.group=group; ++s.observations;
        const QString cls=classify(data,r,valueColumn);
        if(cls=="Blank") ++s.blank;
        else if(cls=="DeclaredMissing") ++s.declaredMissing;
        else if(cls=="NonNumeric") ++s.nonNumeric;
        else { double value; if(numericValue(data,r,valueColumn,value)) groups[group].push_back(value); else ++s.nonNumeric; }
    }
    for(auto it=summaries.cbegin();it!=summaries.cend();++it){
        GroupSummaryRow s=it.value(); const auto x=groups.value(it.key()); s.valid=x.size();
        if(!x.isEmpty()){
            double sum=0; for(double v:x)sum+=v; s.mean=sum/x.size();
            s.minimum=*std::min_element(x.begin(),x.end()); s.maximum=*std::max_element(x.begin(),x.end()); s.median=percentile(x,.5);
            if(x.size()>1){double ss=0;for(double v:x){double d=v-s.mean;ss+=d*d;}s.stdDev=std::sqrt(ss/(x.size()-1));}
        }
        result.push_back(s);
    }
    return result;
}

QString AnalysisEngine::number(double value) { return std::isfinite(value) ? QString::number(value,'f',4) : "—"; }
}
