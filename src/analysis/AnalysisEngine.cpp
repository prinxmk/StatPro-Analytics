#include "AnalysisEngine.h"
#include <algorithm>
#include <QMap>
#include <QSet>

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


namespace {
constexpr double kPi = 3.141592653589793238462643383279502884;
constexpr double kTiny = 1e-300;
constexpr int kMaxIterations = 200;
constexpr double kEps = 3e-14;

static double betaContinuedFraction(double a, double b, double x) {
    double qab=a+b, qap=a+1.0, qam=a-1.0;
    double c=1.0, d=1.0-qab*x/qap; if(std::fabs(d)<kTiny)d=kTiny; d=1.0/d;
    double h=d;
    for(int m=1;m<=kMaxIterations;++m){
        const int m2=2*m;
        double aa=m*(b-m)*x/((qam+m2)*(a+m2));
        d=1.0+aa*d; if(std::fabs(d)<kTiny)d=kTiny; c=1.0+aa/c; if(std::fabs(c)<kTiny)c=kTiny; d=1.0/d; h*=d*c;
        aa=-(a+m)*(qab+m)*x/((a+m2)*(qap+m2));
        d=1.0+aa*d; if(std::fabs(d)<kTiny)d=kTiny; c=1.0+aa/c; if(std::fabs(c)<kTiny)c=kTiny; d=1.0/d;
        const double delta=d*c; h*=delta; if(std::fabs(delta-1.0)<kEps)break;
    }
    return h;
}
}

double AnalysisEngine::logGamma(double x) {
    static const double coeffs[] = {0.99999999999980993,676.5203681218851,-1259.1392167224028,771.32342877765313,-176.61502916214059,12.507343278686905,-0.13857109526572012,9.9843695780195716e-6,1.5056327351493116e-7};
    if(x<0.5) return std::log(kPi)-std::log(std::sin(kPi*x))-logGamma(1.0-x);
    x-=1.0; double a=coeffs[0]; for(int i=1;i<9;++i)a+=coeffs[i]/(x+i);
    const double t=x+7.5; return 0.5*std::log(2.0*kPi)+(x+0.5)*std::log(t)-t+std::log(a);
}

double AnalysisEngine::regularizedBeta(double x,double a,double b){
    if(x<=0)return 0; if(x>=1)return 1;
    const double bt=std::exp(logGamma(a+b)-logGamma(a)-logGamma(b)+a*std::log(x)+b*std::log1p(-x));
    if(x<(a+1.0)/(a+b+2.0)) return bt*betaContinuedFraction(a,b,x)/a;
    return 1.0-bt*betaContinuedFraction(b,a,1.0-x)/b;
}

double AnalysisEngine::regularizedGammaQ(double a,double x){
    if(x<0||a<=0)return NAN; if(x==0)return 1.0;
    if(x<a){ double sum=1.0/a, term=sum; for(int n=1;n<=kMaxIterations;++n){term*=x/(a+n);sum+=term;if(std::fabs(term)<std::fabs(sum)*kEps)break;} return 1.0-std::exp(-x+a*std::log(x)-logGamma(a))*sum; }
    double b=x+1.0-a, c=1.0/kTiny, d=1.0/b; double h=d;
    for(int i=1;i<=kMaxIterations;++i){double an=-i*(i-a);b+=2.0;d=an*d+b;if(std::fabs(d)<kTiny)d=kTiny;c=b+an/c;if(std::fabs(c)<kTiny)c=kTiny;d=1.0/d;const double del=d*c;h*=del;if(std::fabs(del-1.0)<kEps)break;}
    return std::exp(-x+a*std::log(x)-logGamma(a))*h;
}

double AnalysisEngine::normalCdf(double x){ return 0.5*std::erfc(-x/std::sqrt(2.0)); }
double AnalysisEngine::studentTCdf(double t,double df){
    if(!std::isfinite(t)||df<=0)return NAN; if(t==0)return 0.5;
    const double x=df/(df+t*t); const double ib=regularizedBeta(x,df/2.0,0.5); return t>0 ? 1.0-0.5*ib : 0.5*ib;
}
double AnalysisEngine::studentTQuantile(double p,double df){
    if(!std::isfinite(p)||p<=0.0||p>=1.0||df<=0.0)return NAN;
    if(p==0.5)return 0.0;
    const bool neg=p<0.5; const double target=neg?1.0-p:p;
    double lo=0.0, hi=1.0;
    while(studentTCdf(hi,df)<target && hi<1e8) hi*=2.0;
    for(int i=0;i<100;++i){const double mid=(lo+hi)/2.0;if(studentTCdf(mid,df)<target)lo=mid;else hi=mid;}
    return neg?-(lo+hi)/2.0:(lo+hi)/2.0;
}

double AnalysisEngine::chiSquareSurvival(double x,double df){ return (x<0||df<=0)?NAN:regularizedGammaQ(df/2.0,x/2.0); }
double AnalysisEngine::fSurvival(double f,double d1,double d2){ if(f<0||d1<=0||d2<=0)return NAN; return regularizedBeta(d2/(d2+d1*f),d2/2.0,d1/2.0); }

static QVector<int> analysisRows(const DataSet& data,const QVector<int>& rows){
    if(!rows.isEmpty())return rows; QVector<int> r; r.reserve(data.rowCount()); for(int i=0;i<data.rowCount();++i)r.push_back(i); return r;
}
static void accountClass(ObservationAccounting& a,const QString& cls){ if(cls=="Blank")++a.blank; else if(cls=="DeclaredMissing")++a.declaredMissing; else if(cls=="NonNumeric")++a.nonNumeric; }
static double sampleMean(const QVector<double>& x){if(x.isEmpty())return NAN;double s=0;for(double v:x)s+=v;return s/x.size();}
static double sampleSd(const QVector<double>& x){if(x.size()<2)return NAN;double m=sampleMean(x),ss=0;for(double v:x){double d=v-m;ss+=d*d;}return std::sqrt(ss/(x.size()-1));}

CorrelationResult AnalysisEngine::pearsonCorrelation(const DataSet& data,int xColumn,int yColumn,const QVector<int>& rows){
    CorrelationResult out; const auto use=analysisRows(data,rows); out.observations=use.size(); QVector<double>x,y;
    for(int r:use){const QString cx=classify(data,r,xColumn), cy=classify(data,r,yColumn); accountClass(out,cx); accountClass(out,cy); double vx,vy; if(cx=="Valid"&&cy=="Valid"&&numericValue(data,r,xColumn,vx)&&numericValue(data,r,yColumn,vy)){x.push_back(vx);y.push_back(vy);} }
    out.pairs=x.size(); out.valid=out.pairs; if(x.size()<2)return out; double mx=sampleMean(x),my=sampleMean(y),sxx=0,syy=0,sxy=0;for(int i=0;i<x.size();++i){double dx=x[i]-mx,dy=y[i]-my;sxx+=dx*dx;syy+=dy*dy;sxy+=dx*dy;} if(sxx<=0||syy<=0)return out; out.r=sxy/std::sqrt(sxx*syy); out.r=std::max(-1.0,std::min(1.0,out.r));
    if(x.size()>2){const double t=out.r*std::sqrt((x.size()-2)/(1.0-out.r*out.r));out.p=2.0*(1.0-studentTCdf(std::fabs(t),x.size()-2));}
    if(x.size()>3&&std::fabs(out.r)<1.0){const double z=0.5*std::log((1+out.r)/(1-out.r)),se=1.0/std::sqrt(x.size()-3),crit=1.959963984540054;out.ciLow=std::tanh(z-crit*se);out.ciHigh=std::tanh(z+crit*se);} return out;
}

OneSampleTResult AnalysisEngine::oneSampleTTest(const DataSet& data,int column,double testMean,const QVector<int>& rows){
    OneSampleTResult out;out.testMean=testMean;const auto use=analysisRows(data,rows);out.observations=use.size();QVector<double>x;
    for(int r:use){const QString cls=classify(data,r,column);accountClass(out,cls);if(cls=="Valid"){double v;if(numericValue(data,r,column,v))x.push_back(v);else ++out.nonNumeric;}}
    out.valid=x.size();if(x.size()<2)return out;out.mean=sampleMean(x);out.stdDev=sampleSd(x);const double se=out.stdDev/std::sqrt(x.size());if(se<=0)return out;out.t=(out.mean-testMean)/se;out.df=x.size()-1;out.p=2.0*(1.0-studentTCdf(std::fabs(out.t),out.df));double crit=0;
    double lo=0,hi=0; // t critical via a compact numerical search
    double target=0.975,l=0,h=20;for(int i=0;i<80;++i){double mid=(l+h)/2; if(studentTCdf(mid,out.df)<target)l=mid;else h=mid;}crit=(l+h)/2;out.ciLow=(out.mean-testMean)-crit*se;out.ciHigh=(out.mean-testMean)+crit*se;out.cohensD=(out.mean-testMean)/out.stdDev;return out;
}

QStringList AnalysisEngine::independentGroupLevels(const DataSet& data,int groupColumn,const QVector<int>& rows){
    const auto use=analysisRows(data,rows);
    QSet<QString> levels;
    if(groupColumn<0||groupColumn>=data.columnCount()) return {};
    for(int r:use){
        if(classify(data,r,groupColumn)!="Valid") continue;
        QString text=data.value(r,groupColumn).toString().trimmed();
        if(data.variables()[groupColumn].type==VariableType::Numeric){
            bool ok=false; double v=text.toDouble(&ok);
            if(!ok||!std::isfinite(v)) continue;
            text=QString::number(v,'g',15);
        }
        if(!text.isEmpty()) levels.insert(text);
    }
    QStringList out=levels.values();
    std::sort(out.begin(),out.end(),[](const QString&a,const QString&b){return a.localeAwareCompare(b)<0;});
    return out;
}

IndependentTResult AnalysisEngine::independentTTest(const DataSet& data,int groupColumn,int valueColumn,const QString& group1,const QString& group2,bool equalVariances,const QVector<int>& rows){
    IndependentTResult out; const auto use=analysisRows(data,rows); QMap<QString,QVector<double>> vals; QMap<QString,ObservationAccounting> acc;
    if(groupColumn<0||groupColumn>=data.columnCount()||valueColumn<0||valueColumn>=data.columnCount()||groupColumn==valueColumn) return out;
    const QStringList groups=independentGroupLevels(data,groupColumn,rows); out.availableGroups=groups;
    if(groups.size()<2 || group1.isEmpty() || group2.isEmpty() || group1==group2 || !groups.contains(group1) || !groups.contains(group2)) return out;
    acc[group1]=ObservationAccounting{}; acc[group2]=ObservationAccounting{};
    for(int r:use){
        const QString gc=classify(data,r,groupColumn);
        QString g;
        if(gc=="Valid"){
            g=data.value(r,groupColumn).toString().trimmed();
            if(data.variables()[groupColumn].type==VariableType::Numeric){bool ok=false;double v=g.toDouble(&ok);if(ok&&std::isfinite(v))g=QString::number(v,'g',15);else g.clear();}
        }
        if(g.isEmpty()||!acc.contains(g)) continue;
        auto &a=acc[g]; ++a.observations;
        const QString vc=classify(data,r,valueColumn);
        if(vc!="Valid"){accountClass(a,vc);continue;}
        double v;
        if(numericValue(data,r,valueColumn,v)){++a.valid;vals[g].push_back(v);}else ++a.nonNumeric;
    }
    out.group1=group1;out.group2=group2;out.group1Accounting=acc[group1];out.group2Accounting=acc[group2];
    const auto x=vals.value(group1),y=vals.value(group2); out.n1=x.size();out.n2=y.size();
    if(x.size()<2||y.size()<2)return out;
    out.mean1=sampleMean(x);out.mean2=sampleMean(y);out.sd1=sampleSd(x);out.sd2=sampleSd(y);out.difference=out.mean1-out.mean2;
    double se2=0;
    if(equalVariances){
        const double pooled=((x.size()-1)*out.sd1*out.sd1+(y.size()-1)*out.sd2*out.sd2)/(x.size()+y.size()-2);
        se2=pooled*(1.0/x.size()+1.0/y.size()); out.df=x.size()+y.size()-2; out.cohensD=pooled>0?out.difference/std::sqrt(pooled):NAN;
    }else{
        se2=out.sd1*out.sd1/x.size()+out.sd2*out.sd2/y.size();
        const double a=out.sd1*out.sd1/x.size(),b=out.sd2*out.sd2/y.size();
        out.df=(a+b)*(a+b)/(a*a/(x.size()-1)+b*b/(y.size()-1));
        const double pooled=((x.size()-1)*out.sd1*out.sd1+(y.size()-1)*out.sd2*out.sd2)/(x.size()+y.size()-2);
        out.cohensD=pooled>0?out.difference/std::sqrt(pooled):NAN;
    }
    if(se2<=0||!std::isfinite(out.df)||out.df<=0)return out;
    const double se=std::sqrt(se2);out.t=out.difference/se;out.p=2.0*(1.0-studentTCdf(std::fabs(out.t),out.df));
    double l=0,h=20;for(int i=0;i<80;++i){double mid=(l+h)/2;if(studentTCdf(mid,out.df)<0.975)l=mid;else h=mid;}
    const double crit=(l+h)/2;out.ciLow=out.difference-crit*se;out.ciHigh=out.difference+crit*se;return out;
}

PairedTResult AnalysisEngine::pairedTTest(const DataSet& data,int firstColumn,int secondColumn,const QVector<int>& rows){
    PairedTResult out;const auto use=analysisRows(data,rows);out.observations=use.size();QVector<double>d;
    for(int r:use){const QString c1=classify(data,r,firstColumn),c2=classify(data,r,secondColumn);accountClass(out,c1);accountClass(out,c2);double a,b;if(c1=="Valid"&&c2=="Valid"&&numericValue(data,r,firstColumn,a)&&numericValue(data,r,secondColumn,b))d.push_back(a-b);}
    out.pairs=d.size();out.valid=d.size();if(d.size()<2)return out;out.meanDifference=sampleMean(d);out.sdDifference=sampleSd(d);const double se=out.sdDifference/std::sqrt(d.size());if(se<=0)return out;out.t=out.meanDifference/se;out.df=d.size()-1;out.p=2.0*(1.0-studentTCdf(std::fabs(out.t),out.df));double l=0,h=20;for(int i=0;i<80;++i){double mid=(l+h)/2;if(studentTCdf(mid,out.df)<0.975)l=mid;else h=mid;}const double crit=(l+h)/2;out.ciLow=out.meanDifference-crit*se;out.ciHigh=out.meanDifference+crit*se;out.cohensDz=out.meanDifference/out.sdDifference;return out;
}

ChiSquareResult AnalysisEngine::chiSquare(const DataSet& data,int rowColumn,int columnColumn,const QVector<int>& rows){
    ChiSquareResult out;const auto use=analysisRows(data,rows);out.observations=use.size();QMap<QString,int> ri,ci;QVector<QPair<QString,QString>> pairs;
    for(int r:use){const QString a=classify(data,r,rowColumn),b=classify(data,r,columnColumn);accountClass(out,a);accountClass(out,b);if(a!="Valid"||b!="Valid")continue;QString ra=data.value(r,rowColumn).toString().trimmed(),cb=data.value(r,columnColumn).toString().trimmed();if(!ri.contains(ra))ri[ra]=ri.size();if(!ci.contains(cb))ci[cb]=ci.size();pairs.push_back({ra,cb});}
    out.rows=ri.size();out.columns=ci.size();out.valid=pairs.size();if(out.rows<2||out.columns<2)return out;out.rowLabels=QVector<QString>(out.rows);for(auto it=ri.cbegin();it!=ri.cend();++it)out.rowLabels[it.value()]=it.key();out.columnLabels=QVector<QString>(out.columns);for(auto it=ci.cbegin();it!=ci.cend();++it)out.columnLabels[it.value()]=it.key();out.observed=QVector<QVector<double>>(out.rows,QVector<double>(out.columns,0));for(const auto&p:pairs)out.observed[ri[p.first]][ci[p.second]]++;
    QVector<double> rs(out.rows,0),cs(out.columns,0);double total=0;for(int i=0;i<out.rows;++i)for(int j=0;j<out.columns;++j){rs[i]+=out.observed[i][j];cs[j]+=out.observed[i][j];total+=out.observed[i][j];}out.expected=QVector<QVector<double>>(out.rows,QVector<double>(out.columns,0));out.chiSquare=0;for(int i=0;i<out.rows;++i)for(int j=0;j<out.columns;++j){out.expected[i][j]=total>0?rs[i]*cs[j]/total:0;if(out.expected[i][j]>0)out.chiSquare+=std::pow(out.observed[i][j]-out.expected[i][j],2)/out.expected[i][j];}out.df=(out.rows-1)*(out.columns-1);out.p=chiSquareSurvival(out.chiSquare,out.df);out.cramersV=total>0?std::sqrt(out.chiSquare/(total*std::min(out.rows-1,out.columns-1))):NAN;return out;
}

AnovaResult AnalysisEngine::oneWayAnova(const DataSet& data,int groupColumn,int valueColumn,const QVector<int>& rows){
    AnovaResult out; const auto use=analysisRows(data,rows); out.observations=use.size();
    QMap<QString,QVector<double>> vals; QMap<QString,AnovaGroup> stats;
    for(int r:use){
        const QString gc=classify(data,r,groupColumn); QString g;
        if(gc=="Blank") g="(Blank)"; else if(gc=="DeclaredMissing") g="(Declared missing)"; else if(gc=="NonNumeric") g="(Invalid group)"; else g=data.value(r,groupColumn).toString().trimmed();
        auto& s=stats[g]; s.group=g; ++s.observations;
        const QString vc=classify(data,r,valueColumn);
        if(vc=="Valid") { double v; if(numericValue(data,r,valueColumn,v)) vals[g].push_back(v); else { ++s.nonNumeric; ++out.nonNumeric; } }
        else if(vc=="Blank") { ++s.blank; ++out.blank; }
        else if(vc=="DeclaredMissing") { ++s.declaredMissing; ++out.declaredMissing; }
        else { ++s.nonNumeric; ++out.nonNumeric; }
    }
    double totalN=0,sum=0;
    for(auto it=stats.cbegin();it!=stats.cend();++it){
        AnovaGroup s=it.value(); const auto x=vals.value(it.key()); s.valid=x.size();
        if(!x.isEmpty()){s.mean=sampleMean(x);s.stdDev=sampleSd(x);sum+=s.valid*s.mean;totalN+=s.valid;}
        out.groupStats.push_back(s);
    }
    out.valid=static_cast<int>(totalN);
    int testGroups=0; for(const auto& s:out.groupStats) if(!s.group.startsWith("(") && s.valid>0) ++testGroups;
    out.groups=testGroups;
    if(testGroups<2||out.valid<testGroups)return out;
    out.grandMean=sum/totalN; out.ssBetween=0; out.ssWithin=0;
    for(const auto& s:out.groupStats){ if(s.group.startsWith("(")||s.valid<=0) continue; const auto x=vals.value(s.group); out.ssBetween+=s.valid*std::pow(s.mean-out.grandMean,2); for(double v:x) out.ssWithin+=std::pow(v-s.mean,2); }
    out.ssTotal=out.ssBetween+out.ssWithin; out.dfBetween=testGroups-1; out.dfWithin=out.valid-testGroups;
    if(out.dfWithin<=0)return out; out.msBetween=out.ssBetween/out.dfBetween; out.msWithin=out.ssWithin/out.dfWithin; out.f=out.msWithin>0?out.msBetween/out.msWithin:NAN; out.p=std::isfinite(out.f)?fSurvival(out.f,out.dfBetween,out.dfWithin):NAN; out.etaSquared=out.ssTotal>0?out.ssBetween/out.ssTotal:NAN; return out;
}

RegressionResult AnalysisEngine::simpleLinearRegression(const DataSet& data,int xColumn,int yColumn,const QVector<int>& rows){
    RegressionResult out; const auto use=analysisRows(data,rows); out.observations=use.size();
    QVector<double>x,y; x.reserve(use.size()); y.reserve(use.size());
    for(int r:use){
        const QString cx=classify(data,r,xColumn), cy=classify(data,r,yColumn);
        if(cx=="Blank") ++out.xBlank; else if(cx=="DeclaredMissing") ++out.xDeclaredMissing; else if(cx=="NonNumeric") ++out.xNonNumeric;
        if(cy=="Blank") ++out.yBlank; else if(cy=="DeclaredMissing") ++out.yDeclaredMissing; else if(cy=="NonNumeric") ++out.yNonNumeric;
        double vx,vy; if(cx=="Valid"&&cy=="Valid"&&numericValue(data,r,xColumn,vx)&&numericValue(data,r,yColumn,vy)){x.push_back(vx);y.push_back(vy);}
    }
    out.complete=x.size(); if(x.size()<3)return out;
    double mx=sampleMean(x), my=sampleMean(y), sxx=0, syy=0, sxy=0;
    for(int i=0;i<x.size();++i){const double dx=x[i]-mx,dy=y[i]-my;sxx+=dx*dx;syy+=dy*dy;sxy+=dx*dy;}
    if(sxx<=0||syy<0)return out;
    out.slope=sxy/sxx; out.intercept=my-out.slope*mx;
    out.ssTotal=syy; out.ssRegression=out.slope*sxy; out.ssResidual=std::max(0.0,out.ssTotal-out.ssRegression);
    out.dfRegression=1; out.dfResidual=x.size()-2; out.msRegression=out.ssRegression; out.msResidual=out.ssResidual/out.dfResidual;
    out.r=syy>0?std::max(-1.0,std::min(1.0,sxy/std::sqrt(sxx*syy))):NAN;
    out.rSquared=syy>0?std::max(0.0,std::min(1.0,out.ssRegression/syy)):NAN;
    out.adjustedRSquared=1.0-(1.0-out.rSquared)*(x.size()-1.0)/(x.size()-2.0);
    out.rmse=std::sqrt(out.msResidual);
    if(out.msResidual>0){
        out.seSlope=std::sqrt(out.msResidual/sxx);
        out.seIntercept=std::sqrt(out.msResidual*(1.0/x.size()+mx*mx/sxx));
        out.tSlope=out.slope/out.seSlope; out.tIntercept=out.intercept/out.seIntercept;
        out.pSlope=2.0*(1.0-studentTCdf(std::fabs(out.tSlope),out.dfResidual));
        out.pIntercept=2.0*(1.0-studentTCdf(std::fabs(out.tIntercept),out.dfResidual));
        const double crit=studentTQuantile(0.975,out.dfResidual);
        out.slopeCiLow=out.slope-crit*out.seSlope; out.slopeCiHigh=out.slope+crit*out.seSlope;
        out.interceptCiLow=out.intercept-crit*out.seIntercept; out.interceptCiHigh=out.intercept+crit*out.seIntercept;
        out.f=out.ssRegression/out.msResidual; out.fP=fSurvival(out.f,1.0,out.dfResidual);
    } else {
        out.seSlope=0; out.seIntercept=0; out.tSlope=out.slope==0?0:std::copysign(INFINITY,out.slope); out.tIntercept=out.intercept==0?0:std::copysign(INFINITY,out.intercept); out.pSlope=out.slope==0?1.0:0.0; out.pIntercept=out.intercept==0?1.0:0.0; out.slopeCiLow=out.slope; out.slopeCiHigh=out.slope; out.interceptCiLow=out.intercept; out.interceptCiHigh=out.intercept; out.f=INFINITY; out.fP=0.0;
    }
    if(out.ssResidual>0){
        double dwNumerator=0, previous=0; bool first=true;
        for(int i=0;i<x.size();++i){const double e=y[i]-(out.intercept+out.slope*x[i]);if(!first)dwNumerator+=std::pow(e-previous,2);previous=e;first=false;}
        out.durbinWatson=dwNumerator/out.ssResidual;
    }
    return out;
}



static bool invertSquareMatrix(const QVector<QVector<double>>& input, QVector<QVector<double>>& inverse) {
    const int n=input.size();
    if(n==0) return false;
    for(const auto& row:input) if(row.size()!=n) return false;
    QVector<QVector<double>> a(n,QVector<double>(2*n,0.0));
    for(int i=0;i<n;++i){
        for(int j=0;j<n;++j) a[i][j]=input[i][j];
        a[i][n+i]=1.0;
    }
    for(int col=0;col<n;++col){
        int pivot=col;
        double best=std::fabs(a[col][col]);
        for(int r=col+1;r<n;++r){const double v=std::fabs(a[r][col]);if(v>best){best=v;pivot=r;}}
        if(best<1e-12) return false;
        if(pivot!=col) std::swap(a[pivot],a[col]);
        const double d=a[col][col];
        for(int j=0;j<2*n;++j) a[col][j]/=d;
        for(int r=0;r<n;++r){
            if(r==col) continue;
            const double f=a[r][col];
            if(std::fabs(f)<1e-18) continue;
            for(int j=0;j<2*n;++j) a[r][j]-=f*a[col][j];
        }
    }
    inverse=QVector<QVector<double>>(n,QVector<double>(n,0.0));
    for(int i=0;i<n;++i) for(int j=0;j<n;++j) inverse[i][j]=a[i][n+j];
    return true;
}

static double auxiliaryR2(const QVector<QVector<double>>& x, int targetColumn) {
    const int n=x.size();
    if(n<3 || targetColumn<0 || x.isEmpty() || targetColumn>=x[0].size()) return NAN;
    const int k=x[0].size();
    if(k<=1) return 0.0;
    const int q=k; // intercept + (k-1) other predictors
    QVector<QVector<double>> xtx(q,QVector<double>(q,0.0));
    QVector<double> xty(q,0.0), y(n,0.0);
    double mean=0.0;
    for(int i=0;i<n;++i){y[i]=x[i][targetColumn];mean+=y[i];}
    mean/=n;
    for(int i=0;i<n;++i){
        QVector<double> row; row.reserve(q); row.push_back(1.0);
        for(int j=0;j<k;++j) if(j!=targetColumn) row.push_back(x[i][j]);
        for(int a=0;a<q;++a){
            xty[a]+=row[a]*y[i];
            for(int b=0;b<q;++b) xtx[a][b]+=row[a]*row[b];
        }
    }
    QVector<QVector<double>> inv;
    if(!invertSquareMatrix(xtx,inv)) return NAN;
    QVector<double> beta(q,0.0);
    for(int a=0;a<q;++a) for(int b=0;b<q;++b) beta[a]+=inv[a][b]*xty[b];
    double sst=0.0,sse=0.0;
    for(int i=0;i<n;++i){
        double fitted=beta[0]; int pos=1;
        for(int j=0;j<k;++j) if(j!=targetColumn) fitted+=beta[pos++]*x[i][j];
        sst+=std::pow(y[i]-mean,2); sse+=std::pow(y[i]-fitted,2);
    }
    if(sst<=0) return NAN;
    return std::max(0.0,std::min(1.0,1.0-sse/sst));
}

MultipleRegressionResult AnalysisEngine::multipleLinearRegression(const DataSet& data,const QVector<int>& predictorColumns,int yColumn,const QVector<int>& rows){
    MultipleRegressionResult out;
    const auto use=analysisRows(data,rows); out.observations=use.size(); out.predictors=predictorColumns.size();
    if(predictorColumns.isEmpty() || yColumn<0 || yColumn>=data.columnCount()) return out;
    QSet<int> uniquePredictors;
    for(int c:predictorColumns){
        if(c<0 || c>=data.columnCount() || c==yColumn || data.variables()[c].type!=VariableType::Numeric) return out;
        uniquePredictors.insert(c);
    }
    if(uniquePredictors.size()!=predictorColumns.size()) return out;

    QVector<QVector<double>> x;
    QVector<double> y;
    x.reserve(use.size()); y.reserve(use.size());
    for(int r:use){
        bool hasBlank=false,hasDeclared=false,hasInvalid=false;
        const QString yc=classify(data,r,yColumn);
        if(yc=="Blank") hasBlank=true; else if(yc=="DeclaredMissing") hasDeclared=true; else if(yc!="Valid") hasInvalid=true;
        QVector<double> rowX; rowX.reserve(predictorColumns.size());
        for(int c:predictorColumns){
            const QString cls=classify(data,r,c);
            if(cls=="Blank") hasBlank=true; else if(cls=="DeclaredMissing") hasDeclared=true; else if(cls!="Valid") hasInvalid=true;
            double v=NAN; if(cls=="Valid" && numericValue(data,r,c,v)) rowX.push_back(v); else rowX.push_back(NAN);
        }
        double vy=NAN; const bool yOk=(yc=="Valid" && numericValue(data,r,yColumn,vy));
        bool xOk=true; for(double v:rowX) if(!std::isfinite(v)){xOk=false;break;}
        if(yOk && xOk){x.push_back(rowX);y.push_back(vy);}
        else if(hasBlank) ++out.excludedBlank;
        else if(hasDeclared) ++out.excludedDeclaredMissing;
        else ++out.excludedNonNumeric;
    }
    out.complete=y.size();
    const int n=out.complete, k=predictorColumns.size(), p=k+1;
    out.dfRegression=k; out.dfResidual=n-p;
    if(n<=p) return out;

    QVector<QVector<double>> xtx(p,QVector<double>(p,0.0));
    QVector<double> xty(p,0.0);
    for(int i=0;i<n;++i){
        QVector<double> row; row.reserve(p); row.push_back(1.0); for(double v:x[i]) row.push_back(v);
        for(int a=0;a<p;++a){
            xty[a]+=row[a]*y[i];
            for(int b=0;b<p;++b) xtx[a][b]+=row[a]*row[b];
        }
    }
    QVector<QVector<double>> inv;
    if(!invertSquareMatrix(xtx,inv)){out.singular=true;return out;}
    QVector<double> beta(p,0.0);
    for(int a=0;a<p;++a) for(int b=0;b<p;++b) beta[a]+=inv[a][b]*xty[b];

    double yMean=0.0; for(double v:y)yMean+=v; yMean/=n;
    QVector<double> residuals; residuals.reserve(n);
    out.ssTotal=0.0; out.ssResidual=0.0;
    for(int i=0;i<n;++i){
        double fitted=beta[0]; for(int j=0;j<k;++j) fitted+=beta[j+1]*x[i][j];
        const double e=y[i]-fitted; residuals.push_back(e);
        out.ssResidual+=e*e; out.ssTotal+=std::pow(y[i]-yMean,2);
    }
    out.ssRegression=std::max(0.0,out.ssTotal-out.ssResidual);
    out.rSquared=out.ssTotal>0?std::max(0.0,std::min(1.0,1.0-out.ssResidual/out.ssTotal)):NAN;
    out.adjustedRSquared=std::isfinite(out.rSquared)?1.0-(1.0-out.rSquared)*(n-1.0)/out.dfResidual:NAN;
    out.msRegression=out.ssRegression/k; out.msResidual=out.ssResidual/out.dfResidual; out.rmse=std::sqrt(std::max(0.0,out.msResidual));
    if(out.msResidual>0){out.f=out.msRegression/out.msResidual;out.fP=fSurvival(out.f,k,out.dfResidual);}else{out.f=INFINITY;out.fP=0.0;}
    if(out.ssResidual>0){double num=0.0;for(int i=1;i<residuals.size();++i)num+=std::pow(residuals[i]-residuals[i-1],2);out.durbinWatson=num/out.ssResidual;}

    double ySS=0.0; for(double v:y)ySS+=std::pow(v-yMean,2); const double ySd=n>1?std::sqrt(ySS/(n-1)):NAN;
    QVector<double> xSd(k,NAN);
    for(int j=0;j<k;++j){double m=0.0;for(const auto& row:x)m+=row[j];m/=n;double ss=0.0;for(const auto& row:x)ss+=std::pow(row[j]-m,2);if(n>1)xSd[j]=std::sqrt(ss/(n-1));}
    const double crit=studentTQuantile(0.975,out.dfResidual);
    for(int a=0;a<p;++a){
        MultipleRegressionCoefficient c;
        c.term=(a==0)?"Intercept":data.variables()[predictorColumns[a-1]].name;
        c.estimate=beta[a];
        c.stdError=std::sqrt(std::max(0.0,out.msResidual*inv[a][a]));
        if(c.stdError>0){c.t=c.estimate/c.stdError;c.p=2.0*(1.0-studentTCdf(std::fabs(c.t),out.dfResidual));c.ciLow=c.estimate-crit*c.stdError;c.ciHigh=c.estimate+crit*c.stdError;}
        else {c.t=c.estimate==0?0:std::copysign(INFINITY,c.estimate);c.p=c.estimate==0?1.0:0.0;c.ciLow=c.estimate;c.ciHigh=c.estimate;}
        if(a>0){
            c.standardizedBeta=(std::isfinite(ySd)&&ySd>0)?c.estimate*xSd[a-1]/ySd:NAN;
            const double aux=auxiliaryR2(x,a-1);
            c.vif=std::isfinite(aux)&&aux<1.0?1.0/(1.0-aux):(aux>=1.0?INFINITY:NAN);
        }
        out.coefficients.push_back(c);
    }
    return out;
}



MultipleRegressionResult AnalysisEngine::regressionWithCategoricalPredictors(const DataSet& data,const QVector<int>& predictorColumns,int yColumn,const QVector<int>& rows){
    MultipleRegressionResult out;
    const auto use=analysisRows(data,rows); out.observations=use.size(); out.predictors=predictorColumns.size();
    if(predictorColumns.isEmpty() || yColumn<0 || yColumn>=data.columnCount()) return out;
    QSet<int> uniquePredictors;
    for(int c:predictorColumns){
        if(c<0 || c>=data.columnCount() || c==yColumn) return out;
        if(data.variables()[c].type!=VariableType::Numeric && data.variables()[c].type!=VariableType::String && data.variables()[c].type!=VariableType::Boolean) return out;
        uniquePredictors.insert(c);
    }
    if(uniquePredictors.size()!=predictorColumns.size() || data.variables()[yColumn].type!=VariableType::Numeric) return out;

    struct ExpandedTerm { QString label; int sourceColumn{-1}; QString level; QString reference; };
    QVector<ExpandedTerm> terms;
    for(int c:predictorColumns){
        const auto type=data.variables()[c].type;
        if(type==VariableType::Numeric){ terms.push_back({data.variables()[c].name,c,QString(),QString()}); continue; }
        QSet<QString> levelSet;
        for(int r:use){
            const QString cls=classify(data,r,c);
            if(cls=="Valid") levelSet.insert(data.value(r,c).toString().trimmed());
        }
        QStringList levels=levelSet.values(); std::sort(levels.begin(),levels.end(),[](const QString&a,const QString&b){return QString::localeAwareCompare(a,b)<0;});
        if(levels.size()<2) return out;
        const QString ref=levels.first();
        for(int i=1;i<levels.size();++i) terms.push_back({data.variables()[c].name+" ["+levels[i]+" vs "+ref+"]",c,levels[i],ref});
    }
    if(terms.isEmpty()) return out;

    QVector<QVector<double>> x; QVector<double> y; int excludedBlank=0,excludedDeclared=0,excludedInvalid=0;
    for(int r:use){
        bool hasBlank=false,hasDeclared=false,hasInvalid=false;
        const QString yc=classify(data,r,yColumn);
        if(yc=="Blank") hasBlank=true; else if(yc=="DeclaredMissing") hasDeclared=true; else if(yc!="Valid") hasInvalid=true;
        QVector<double> row; row.reserve(terms.size()); bool okRow=(yc=="Valid"); double vy=NAN;
        if(okRow) okRow=numericValue(data,r,yColumn,vy);
        for(const auto& term:terms){
            const QString cls=classify(data,r,term.sourceColumn);
            if(cls=="Blank") hasBlank=true; else if(cls=="DeclaredMissing") hasDeclared=true; else if(cls!="Valid") hasInvalid=true;
            if(cls!="Valid"){row.push_back(NAN);okRow=false;continue;}
            if(data.variables()[term.sourceColumn].type==VariableType::Numeric){double v=NAN;if(!numericValue(data,r,term.sourceColumn,v)){okRow=false;row.push_back(NAN);}else row.push_back(v);}
            else {const QString level=data.value(r,term.sourceColumn).toString().trimmed(); row.push_back(level==term.level?1.0:0.0);}
        }
        if(okRow){x.push_back(row);y.push_back(vy);} else if(hasBlank) ++excludedBlank; else if(hasDeclared) ++excludedDeclared; else ++excludedInvalid;
    }
    out.complete=y.size(); out.excludedBlank=excludedBlank; out.excludedDeclaredMissing=excludedDeclared; out.excludedNonNumeric=excludedInvalid;
    const int n=out.complete, k=terms.size(), p=k+1;
    if(n<=p) return out;

    QVector<QVector<double>> xtx(p,QVector<double>(p,0.0)); QVector<double> xty(p,0.0); QVector<QVector<double>> design; design.reserve(n);
    for(int i=0;i<n;++i){
        QVector<double> row; row.reserve(p); row.push_back(1.0); for(double v:x[i]) row.push_back(v); design.push_back(row);
        for(int a=0;a<p;++a){xty[a]+=row[a]*y[i];for(int b=0;b<p;++b)xtx[a][b]+=row[a]*row[b];}
    }
    QVector<QVector<double>> inv; if(!invertSquareMatrix(xtx,inv)){out.singular=true;return out;}
    QVector<double> beta(p,0.0); for(int a=0;a<p;++a)for(int b=0;b<p;++b)beta[a]+=inv[a][b]*xty[b];
    double yMean=0.0;for(double v:y)yMean+=v;yMean/=n;
    double sst=0.0,sse=0.0,dwNumerator=0.0; QVector<double> residuals(n,0.0); QVector<double> ySdTerm(terms.size(),NAN);
    for(int j=0;j<k;++j){double mean=0;for(int i=0;i<n;++i)mean+=x[i][j];mean/=n;double ss=0;for(int i=0;i<n;++i)ss+=std::pow(x[i][j]-mean,2);if(n>1)ySdTerm[j]=std::sqrt(ss/(n-1));}
    for(int i=0;i<n;++i){double fit=0;for(int a=0;a<p;++a)fit+=design[i][a]*beta[a];residuals[i]=y[i]-fit;sse+=residuals[i]*residuals[i];sst+=std::pow(y[i]-yMean,2);if(i>0)dwNumerator+=std::pow(residuals[i]-residuals[i-1],2);}
    out.ssTotal=sst; out.ssResidual=sse; out.ssRegression=std::max(0.0,sst-sse); out.dfRegression=k; out.dfResidual=n-p;
    if(out.dfResidual<=0)return out; out.msResidual=sse/out.dfResidual; out.msRegression=k>0?out.ssRegression/k:NAN; out.rmse=std::sqrt(std::max(0.0,out.msResidual));
    out.rSquared=sst>0?std::max(0.0,std::min(1.0,1.0-sse/sst)):NAN; out.adjustedRSquared=std::isfinite(out.rSquared)?1.0-(1.0-out.rSquared)*(n-1.0)/(n-p):NAN;
    out.f=(out.msResidual>0&&std::isfinite(out.msRegression))?out.msRegression/out.msResidual:NAN; out.fP=std::isfinite(out.f)?fSurvival(out.f,out.dfRegression,out.dfResidual):NAN; out.durbinWatson=sse>0?dwNumerator/sse:NAN;
    const double crit=studentTQuantile(0.975,out.dfResidual);
    for(int a=0;a<p;++a){
        MultipleRegressionCoefficient c; c.term=(a==0)?"Intercept":terms[a-1].label; c.estimate=beta[a]; c.stdError=std::sqrt(std::max(0.0,out.msResidual*inv[a][a]));
        if(c.stdError>0){c.t=c.estimate/c.stdError;c.p=2.0*(1.0-studentTCdf(std::fabs(c.t),out.dfResidual));c.ciLow=c.estimate-crit*c.stdError;c.ciHigh=c.estimate+crit*c.stdError;}else{c.t=c.estimate==0?0:std::copysign(INFINITY,c.estimate);c.p=c.estimate==0?1.0:0.0;c.ciLow=c.estimate;c.ciHigh=c.estimate;}
        if(a>0){c.standardizedBeta=(std::isfinite(ySdTerm[a-1])&&ySdTerm[a-1]>0&&n>1&&sst>0)?c.estimate*ySdTerm[a-1]/std::sqrt(sst/(n-1)):NAN;const double aux=auxiliaryR2(x,a-1);c.vif=std::isfinite(aux)&&aux<1.0?1.0/(1.0-aux):(aux>=1.0?INFINITY:NAN);}
        out.coefficients.push_back(c);
    }
    return out;
}


RegressionDiagnosticsResult AnalysisEngine::regressionDiagnostics(const DataSet& data,const QVector<int>& predictorColumns,int yColumn,const QVector<int>& rows){
    RegressionDiagnosticsResult out;
    const auto use=analysisRows(data,rows);
    out.observations=use.size(); out.predictors=predictorColumns.size(); out.parameters=predictorColumns.size()+1;
    if(predictorColumns.isEmpty() || yColumn<0 || yColumn>=data.columnCount()) return out;
    QSet<int> uniquePredictors;
    for(int c:predictorColumns){
        if(c<0 || c>=data.columnCount() || c==yColumn || data.variables()[c].type!=VariableType::Numeric) return out;
        uniquePredictors.insert(c);
    }
    if(uniquePredictors.size()!=predictorColumns.size()) return out;

    QVector<QVector<double>> x; QVector<double> y; QVector<int> sourceRows;
    for(int r:use){
        bool hasBlank=false,hasDeclared=false,hasInvalid=false;
        const QString yc=classify(data,r,yColumn);
        if(yc=="Blank") hasBlank=true; else if(yc=="DeclaredMissing") hasDeclared=true; else if(yc!="Valid") hasInvalid=true;
        QVector<double> rowX; rowX.reserve(predictorColumns.size());
        for(int c:predictorColumns){
            const QString cls=classify(data,r,c);
            if(cls=="Blank") hasBlank=true; else if(cls=="DeclaredMissing") hasDeclared=true; else if(cls!="Valid") hasInvalid=true;
            double v=NAN;
            if(cls=="Valid" && numericValue(data,r,c,v)) rowX.push_back(v); else rowX.push_back(NAN);
        }
        double vy=NAN; const bool yOk=(yc=="Valid" && numericValue(data,r,yColumn,vy));
        bool xOk=true; for(double v:rowX) if(!std::isfinite(v)){xOk=false;break;}
        if(yOk && xOk){x.push_back(rowX); y.push_back(vy); sourceRows.push_back(r);}
        else if(hasBlank) ++out.excludedBlank; else if(hasDeclared) ++out.excludedDeclaredMissing; else ++out.excludedNonNumeric;
    }
    out.complete=y.size();
    const int n=out.complete, k=predictorColumns.size(), p=k+1;
    if(n<=p) return out;

    QVector<QVector<double>> xtx(p,QVector<double>(p,0.0));
    QVector<double> xty(p,0.0);
    QVector<QVector<double>> design; design.reserve(n);
    for(int i=0;i<n;++i){
        QVector<double> row; row.reserve(p); row.push_back(1.0); for(double v:x[i]) row.push_back(v); design.push_back(row);
        for(int a=0;a<p;++a){ xty[a]+=row[a]*y[i]; for(int b=0;b<p;++b) xtx[a][b]+=row[a]*row[b]; }
    }
    QVector<QVector<double>> inv;
    if(!invertSquareMatrix(xtx,inv)){out.singular=true;return out;}
    QVector<double> beta(p,0.0); for(int a=0;a<p;++a) for(int b=0;b<p;++b) beta[a]+=inv[a][b]*xty[b];

    double yMean=0.0; for(double v:y)yMean+=v; yMean/=n;
    QVector<double> residuals(n,0.0), fitted(n,0.0), leverage(n,0.0);
    double sse=0.0,sst=0.0,dwNum=0.0;
    for(int i=0;i<n;++i){
        double fit=0.0; for(int a=0;a<p;++a) fit+=design[i][a]*beta[a];
        fitted[i]=fit; residuals[i]=y[i]-fit; sse+=residuals[i]*residuals[i]; sst+=std::pow(y[i]-yMean,2);
        double h=0.0; for(int a=0;a<p;++a) for(int b=0;b<p;++b) h+=design[i][a]*inv[a][b]*design[i][b];
        leverage[i]=std::max(0.0,std::min(1.0,h));
        if(i>0) dwNum+=std::pow(residuals[i]-residuals[i-1],2);
    }
    out.mse=sse/(n-p); out.rmse=std::sqrt(std::max(0.0,out.mse));
    out.rSquared=sst>0?std::max(0.0,std::min(1.0,1.0-sse/sst)):NAN;
    out.adjustedRSquared=std::isfinite(out.rSquared)?1.0-(1.0-out.rSquared)*(n-1.0)/(n-p):NAN;
    out.durbinWatson=sse>0?dwNum/sse:NAN;

    const double highLevThreshold=2.0*p/static_cast<double>(n);
    const double cookThreshold=4.0/static_cast<double>(n);
    QVector<double> zResiduals; zResiduals.reserve(n);
    double maxAbsStudent=0.0;
    for(int i=0;i<n;++i){
        const double h=leverage[i];
        const double denom=std::sqrt(std::max(kTiny,out.mse*(1.0-h)));
        const double standardized=residuals[i]/denom;
        double externalMse=NAN;
        const double denomLeave=(1.0-h);
        if(n-p-1>0 && denomLeave>1e-12) externalMse=std::max(0.0,(sse-residuals[i]*residuals[i]/denomLeave)/(n-p-1.0));
        const double studentized=(std::isfinite(externalMse)&&externalMse>0)?residuals[i]/std::sqrt(externalMse*denomLeave):standardized;
        const double cook=(h<1.0 && out.mse>0)?(residuals[i]*residuals[i]/(p*out.mse))*(h/std::pow(1.0-h,2)):INFINITY;
        RegressionDiagnosticRow d; d.observation=sourceRows[i]+1; d.actual=y[i]; d.predicted=fitted[i]; d.residual=residuals[i]; d.standardizedResidual=standardized; d.studentizedResidual=studentized; d.leverage=h; d.cooksDistance=cook;
        d.highLeverage=(h>highLevThreshold); d.influential=(cook>cookThreshold); d.largeResidual=(std::fabs(studentized)>2.0);
        if(d.highLeverage)++out.highLeverageCount; if(d.influential)++out.influentialCount; if(d.largeResidual)++out.largeResidualCount;
        out.maxLeverage=std::isfinite(out.maxLeverage)?std::max(out.maxLeverage,h):h;
        out.maxCooksDistance=std::isfinite(out.maxCooksDistance)?std::max(out.maxCooksDistance,cook):cook;
        zResiduals.push_back(standardized); maxAbsStudent=std::max(maxAbsStudent,std::fabs(studentized)); out.rows.push_back(d);
    }
    if(n>3){
        double s3=0.0,s4=0.0;
        for(double z:zResiduals){s3+=std::pow(z,3);s4+=std::pow(z,4);}
        const double skew=s3/n; const double excess=s4/n-3.0;
        out.jarqueBera=n/6.0*(skew*skew+0.25*excess*excess);
        out.jarqueBeraP=chiSquareSurvival(out.jarqueBera,2.0);
    }
    return out;
}


RegressionPredictionResult AnalysisEngine::regressionPrediction(const DataSet& data,int xColumn,int yColumn,int movingWindow,const QVector<int>& rows){
    RegressionPredictionResult out;
    const auto use=analysisRows(data,rows); out.observations=use.size();
    QVector<double> x,y; QVector<int> src;
    for(int r:use){double xv,yv; if(numericValue(data,r,xColumn,xv)&&numericValue(data,r,yColumn,yv)){x.push_back(xv);y.push_back(yv);src.push_back(r);}}
    out.complete=x.size(); if(x.size()<3)return out;
    out.meanX=sampleMean(x); out.sxx=0.0; for(double v:x)out.sxx+=std::pow(v-out.meanX,2);
    if(out.sxx<=0)return out;
    const double meanY=sampleMean(y); double sxy=0,syy=0; for(int i=0;i<x.size();++i){sxy+=(x[i]-out.meanX)*(y[i]-meanY);syy+=std::pow(y[i]-meanY,2);} out.slope=sxy/out.sxx; out.intercept=meanY-out.slope*out.meanX;
    double sse=0; for(int i=0;i<x.size();++i){double e=y[i]-(out.intercept+out.slope*x[i]);sse+=e*e;} out.dfResidual=x.size()-2; out.rmse=std::sqrt(std::max(0.0,sse/out.dfResidual)); out.rSquared=syy>0?std::max(0.0,std::min(1.0,1.0-sse/syy)):NAN;
    const double crit=studentTQuantile(0.975,out.dfResidual); const int w=std::max(1,movingWindow); out.movingWindow=w;
    for(int i=0;i<x.size();++i){RegressionPredictionRow row;row.observation=src[i]+1;row.x=x[i];row.actual=y[i];row.fitted=out.intercept+out.slope*x[i];row.residual=y[i]-row.fitted;const double seMean=out.rmse*std::sqrt(1.0/x.size()+std::pow(x[i]-out.meanX,2)/out.sxx);const double sePred=out.rmse*std::sqrt(1.0+1.0/x.size()+std::pow(x[i]-out.meanX,2)/out.sxx);row.meanCiLow=row.fitted-crit*seMean;row.meanCiHigh=row.fitted+crit*seMean;row.predictionLow=row.fitted-crit*sePred;row.predictionHigh=row.fitted+crit*sePred;out.rows.push_back(row);}
    return out;
}

LogisticRegressionResult AnalysisEngine::logisticRegression(const DataSet& data,int yColumn,const QVector<int>& predictorColumns,const QVector<int>& rows){
    LogisticRegressionResult out; const auto use=analysisRows(data,rows); out.observations=use.size(); out.predictors=predictorColumns.size(); out.parameters=predictorColumns.size()+1;
    if(yColumn<0||yColumn>=data.columnCount()||predictorColumns.isEmpty())return out;
    QSet<int> seen; for(int c:predictorColumns){if(c<0||c>=data.columnCount()||c==yColumn||data.variables()[c].type!=VariableType::Numeric)return out;seen.insert(c);} if(seen.size()!=predictorColumns.size())return out;
    QVector<QVector<double>> X; QVector<double> Y; int eb=0,em=0,ei=0;
    for(int r:use){bool blank=false,missing=false,invalid=false; const QString yc=classify(data,r,yColumn); if(yc=="Blank")blank=true;else if(yc=="DeclaredMissing")missing=true;else if(yc!="Valid")invalid=true; double yv=NAN; bool yok=false;
        if(yc=="Valid"){QString t=data.value(r,yColumn).toString().trimmed();bool ok=false;yv=t.toDouble(&ok);if(ok&&std::isfinite(yv)&&(yv==0.0||yv==1.0))yok=true;else {const QString q=t.toLower();if(q=="true"||q=="yes"||q=="success"||q=="positive"){yv=1;yok=true;}else if(q=="false"||q=="no"||q=="failure"||q=="negative"){yv=0;yok=true;}else invalid=true;}}
        QVector<double> row;row.reserve(predictorColumns.size());bool xok=true;for(int c:predictorColumns){const QString cls=classify(data,r,c);if(cls=="Blank")blank=true;else if(cls=="DeclaredMissing")missing=true;else if(cls!="Valid")invalid=true;double v=NAN;if(cls=="Valid"&&numericValue(data,r,c,v))row.push_back(v);else{xok=false;row.push_back(NAN);}}
        if(yok&&xok){X.push_back(row);Y.push_back(yv);}else if(blank)++eb;else if(missing)++em;else ++ei;
    }
    out.complete=Y.size();out.excludedBlank=eb;out.excludedDeclaredMissing=em;out.excludedNonNumeric=ei;const int n=out.complete,k=predictorColumns.size(),p=k+1;if(n<=p)return out;
    QVector<double> beta(p,0.0); QVector<QVector<double>> cov; bool converged=false;
    auto sigmoid=[](double z){if(z>=0){double e=std::exp(-z);return 1.0/(1.0+e);}double e=std::exp(z);return e/(1.0+e);};
    for(int iter=0;iter<100;++iter){QVector<QVector<double>> h(p,QVector<double>(p,0));QVector<double> g(p,0);for(int i=0;i<n;++i){QVector<double> z(p);z[0]=1;for(int j=0;j<k;++j)z[j+1]=X[i][j];double eta=0;for(int a=0;a<p;++a)eta+=beta[a]*z[a];eta=std::max(-30.0,std::min(30.0,eta));double pr=sigmoid(eta),w=std::max(1e-8,pr*(1-pr));for(int a=0;a<p;++a){g[a]+=z[a]*(Y[i]-pr);for(int b=0;b<p;++b)h[a][b]+=w*z[a]*z[b];}}QVector<QVector<double>> inv;if(!invertSquareMatrix(h,inv)){out.singular=true;return out;}QVector<double> step(p,0);for(int a=0;a<p;++a)for(int b=0;b<p;++b)step[a]+=inv[a][b]*g[b];double maxStep=0;for(int a=0;a<p;++a){beta[a]+=step[a];maxStep=std::max(maxStep,std::fabs(step[a]));}out.iterations=iter+1;if(maxStep<1e-7){converged=true;cov=inv;break;}}
    out.converged=converged;if(!converged)return out;
    double ll=0,nullLL=0;int tp=0,tn=0,fp=0,fn=0;double meanY=sampleMean(Y);
    for(int i=0;i<n;++i){double eta=beta[0];for(int j=0;j<k;++j)eta+=beta[j+1]*X[i][j];eta=std::max(-30.0,std::min(30.0,eta));double pr=sigmoid(eta);ll+=Y[i]*std::log(std::max(1e-15,pr))+(1-Y[i])*std::log(std::max(1e-15,1-pr));int pred=pr>=0.5?1:0;if(pred&&Y[i]==1)++tp;else if(!pred&&Y[i]==0)++tn;else if(pred)++fp;else ++fn;}nullLL=n*(meanY>0&&meanY<1?(meanY*std::log(meanY)+(1-meanY)*std::log(1-meanY)):0.0);out.logLikelihood=ll;out.nullLogLikelihood=nullLL;out.minus2LogLikelihood=-2*ll;out.aic=-2*ll+2*p;out.bic=-2*ll+p*std::log(n);out.mcfaddenR2=(nullLL!=0)?1.0-ll/nullLL:NAN;out.truePositive=tp;out.trueNegative=tn;out.falsePositive=fp;out.falseNegative=fn;out.accuracy=static_cast<double>(tp+tn)/n;out.sensitivity=(tp+fn)>0?static_cast<double>(tp)/(tp+fn):NAN;out.specificity=(tn+fp)>0?static_cast<double>(tn)/(tn+fp):NAN;
    const double crit=1.95996398454;for(int a=0;a<p;++a){LogisticCoefficient c;c.term=a==0?"Intercept":data.variables()[predictorColumns[a-1]].name;c.estimate=beta[a];c.stdError=std::sqrt(std::max(0.0,cov[a][a]));if(c.stdError>0){c.z=c.estimate/c.stdError;c.p=2.0*(1.0-normalCdf(std::fabs(c.z)));c.oddsRatio=std::exp(std::max(-700.0,std::min(700.0,c.estimate)));c.ciLow=std::exp(std::max(-700.0,std::min(700.0,c.estimate-crit*c.stdError)));c.ciHigh=std::exp(std::max(-700.0,std::min(700.0,c.estimate+crit*c.stdError)));}out.coefficients.push_back(c);}return out;
}

TimeSeriesResult AnalysisEngine::timeSeriesAnalysis(const DataSet& data,int timeColumn,int valueColumn,int movingWindow,const QVector<int>& rows){
    TimeSeriesResult out;const auto use=analysisRows(data,rows);out.observations=use.size();out.movingWindow=std::max(1,movingWindow);QVector<double> y;QVector<int> src;QVector<QString> labels;
    for(int r:use){const QString cls=classify(data,r,valueColumn);if(cls=="Blank")++out.excludedBlank;else if(cls=="DeclaredMissing")++out.excludedDeclaredMissing;else if(cls!="Valid")++out.excludedNonNumeric;double v;if(cls=="Valid"&&numericValue(data,r,valueColumn,v)){y.push_back(v);src.push_back(r);labels.push_back(data.value(r,timeColumn).toString().trimmed());}}
    out.valid=y.size();if(y.isEmpty())return out;out.mean=sampleMean(y);out.min=*std::min_element(y.begin(),y.end());out.max=*std::max_element(y.begin(),y.end());out.stdDev=y.size()>1?sampleSd(y):0;
    double mt=0,my=out.mean;for(int i=0;i<y.size();++i)mt+=i;mt/=y.size();double sxx=0,sxy=0,syy=0;for(int i=0;i<y.size();++i){double dx=i-mt,dy=y[i]-my;sxx+=dx*dx;sxy+=dx*dy;syy+=dy*dy;}if(sxx>0){out.trendSlope=sxy/sxx;double sse=syy-sxy*sxy/sxx;out.trendR2=syy>0?std::max(0.0,std::min(1.0,1.0-sse/syy)):NAN;}
    QVector<double> diffs;for(int i=0;i<y.size();++i){TimeSeriesRow r;r.observation=src[i]+1;r.time=labels[i];r.value=y[i];if(i>0){r.lag1=y[i-1];r.difference=y[i]-y[i-1];if(y[i-1]!=0)r.percentChange=100.0*(y[i]-y[i-1])/std::fabs(y[i-1]);diffs.push_back(r.difference);}int start=std::max(0,i-out.movingWindow+1);double sum=0;for(int j=start;j<=i;++j)sum+=y[j];r.movingAverage=sum/(i-start+1);out.rows.push_back(r);}
    if(!diffs.isEmpty()){out.firstDifferenceMean=sampleMean(diffs);out.firstDifferenceSd=diffs.size()>1?sampleSd(diffs):0;}if(y.size()>2){double num=0,den=0;for(int i=1;i<y.size();++i)num+=(y[i]-my)*(y[i-1]-my);for(double v:y)den+=(v-my)*(v-my);out.acf1=den>0?num/den:NAN;}return out;
}

EconometricResult AnalysisEngine::econometricRobustOls(const DataSet& data,int yColumn,const QVector<int>& predictorColumns,const QVector<int>& rows){
    EconometricResult out;const auto use=analysisRows(data,rows);out.observations=use.size();out.predictors=predictorColumns.size();out.parameters=predictorColumns.size()+1;if(yColumn<0||yColumn>=data.columnCount()||predictorColumns.isEmpty())return out;QSet<int> seen;for(int c:predictorColumns){if(c<0||c>=data.columnCount()||c==yColumn||data.variables()[c].type!=VariableType::Numeric)return out;seen.insert(c);}if(seen.size()!=predictorColumns.size())return out;
    QVector<QVector<double>> X;QVector<double> Y;int eb=0,em=0,ei=0;for(int r:use){bool b=false,m=false,iv=false;QString yc=classify(data,r,yColumn);if(yc=="Blank")b=true;else if(yc=="DeclaredMissing")m=true;else if(yc!="Valid")iv=true;double vy=NAN;bool ok=yc=="Valid"&&numericValue(data,r,yColumn,vy);QVector<double> row;for(int c:predictorColumns){QString cls=classify(data,r,c);if(cls=="Blank")b=true;else if(cls=="DeclaredMissing")m=true;else if(cls!="Valid")iv=true;double v=NAN;if(cls=="Valid"&&numericValue(data,r,c,v))row.push_back(v);else{ok=false;row.push_back(NAN);}}if(ok){X.push_back(row);Y.push_back(vy);}else if(b)++eb;else if(m)++em;else ++ei;}out.complete=Y.size();out.excludedBlank=eb;out.excludedDeclaredMissing=em;out.excludedNonNumeric=ei;int n=out.complete,k=out.predictors,p=k+1;if(n<=p)return out;
    QVector<QVector<double>> xtx(p,QVector<double>(p,0));QVector<double> xty(p,0);QVector<QVector<double>> D;for(int i=0;i<n;++i){QVector<double> z{1.0};for(double v:X[i])z.push_back(v);D.push_back(z);for(int a=0;a<p;++a){xty[a]+=z[a]*Y[i];for(int b=0;b<p;++b)xtx[a][b]+=z[a]*z[b];}}QVector<QVector<double>> inv;if(!invertSquareMatrix(xtx,inv)){out.singular=true;return out;}QVector<double> beta(p);for(int a=0;a<p;++a)for(int b=0;b<p;++b)beta[a]+=inv[a][b]*xty[b];double ym=sampleMean(Y),sse=0,sst=0;QVector<double> e(n);for(int i=0;i<n;++i){double fit=0;for(int a=0;a<p;++a)fit+=D[i][a]*beta[a];e[i]=Y[i]-fit;sse+=e[i]*e[i];sst+=std::pow(Y[i]-ym,2);}out.rSquared=sst>0?std::max(0.0,std::min(1.0,1-sse/sst)):NAN;out.adjustedRSquared=std::isfinite(out.rSquared)?1-(1-out.rSquared)*(n-1.0)/(n-p):NAN;out.rmse=std::sqrt(sse/(n-p));out.f=(k>0&&out.rmse>0)?((sst-sse)/k)/(sse/(n-p)):NAN;out.fP=std::isfinite(out.f)?fSurvival(out.f,k,n-p):NAN;double dw=0;for(int i=1;i<n;++i)dw+=std::pow(e[i]-e[i-1],2);out.durbinWatson=sse>0?dw/sse:NAN;
    QVector<QVector<double>> meat(p,QVector<double>(p,0));for(int i=0;i<n;++i)for(int a=0;a<p;++a)for(int b=0;b<p;++b)meat[a][b]+=e[i]*e[i]*D[i][a]*D[i][b];QVector<QVector<double>> tmp(p,QVector<double>(p,0)),rob(p,QVector<double>(p,0));for(int a=0;a<p;++a)for(int b=0;b<p;++b)for(int c=0;c<p;++c)tmp[a][b]+=inv[a][c]*meat[c][b];for(int a=0;a<p;++a)for(int b=0;b<p;++b)for(int c=0;c<p;++c)rob[a][b]+=tmp[a][c]*inv[c][b];double hc1=static_cast<double>(n)/(n-p);const double crit=1.95996398454;for(int a=0;a<p;++a){EconometricCoefficient c;c.term=a==0?"Intercept":data.variables()[predictorColumns[a-1]].name;c.estimate=beta[a];c.robustStdError=std::sqrt(std::max(0.0,rob[a][a]*hc1));if(c.robustStdError>0){c.zOrT=c.estimate/c.robustStdError;c.pValue=2*(1-normalCdf(std::fabs(c.zOrT)));c.ciLow=c.estimate-crit*c.robustStdError;c.ciHigh=c.estimate+crit*c.robustStdError;}out.coefficients.push_back(c);}
    // Breusch-Pagan LM test: regress squared residuals on the original predictors and intercept.
    QVector<double> e2(n);for(int i=0;i<n;++i)e2[i]=e[i]*e[i];double emean=sampleMean(e2);QVector<double> sseAux=e2;QVector<double> xty2(p,0);for(int i=0;i<n;++i)for(int a=0;a<p;++a)xty2[a]+=D[i][a]*e2[i];QVector<double> ba(p);for(int a=0;a<p;++a)for(int b=0;b<p;++b)ba[a]+=inv[a][b]*xty2[b];double sst2=0,sse2=0;for(int i=0;i<n;++i){double fit=0;for(int a=0;a<p;++a)fit+=D[i][a]*ba[a];sst2+=std::pow(e2[i]-emean,2);sse2+=std::pow(e2[i]-fit,2);}double r2aux=sst2>0?std::max(0.0,1-sse2/sst2):0;out.breuschPaganLM=n*r2aux;out.breuschPaganP=chiSquareSurvival(out.breuschPaganLM,k);return out;
}

ADFResult AnalysisEngine::augmentedDickeyFuller(const DataSet& data,int valueColumn,const QVector<int>& rows){
    ADFResult out;const auto use=analysisRows(data,rows);out.observations=use.size();QVector<double> y;for(int r:use){double v;if(numericValue(data,r,valueColumn,v))y.push_back(v);}out.valid=y.size();if(y.size()<6)return out;const int n=y.size()-1;double mx=0,my=0;QVector<double> lag(n),diff(n);for(int i=1;i<y.size();++i){lag[i-1]=y[i-1];diff[i-1]=y[i]-y[i-1];mx+=lag[i-1];my+=diff[i-1];}mx/=n;my/=n;double sxx=0,sxy=0;for(int i=0;i<n;++i){sxx+=std::pow(lag[i]-mx,2);sxy+=(lag[i]-mx)*(diff[i]-my);}if(sxx<=0)return out;double gamma=sxy/sxx,alpha=my-gamma*mx,sse=0;for(int i=0;i<n;++i)sse+=std::pow(diff[i]-(alpha+gamma*lag[i]),2);double seGamma=std::sqrt((sse/(n-2))/sxx);out.statistic=gamma/seGamma;out.critical1=-3.43;out.critical5=-2.86;out.critical10=-2.57;if(out.statistic<out.critical1)out.conclusion="Reject unit-root null at 1% level (strong evidence of stationarity).";else if(out.statistic<out.critical5)out.conclusion="Reject unit-root null at 5% level (evidence of stationarity).";else if(out.statistic<out.critical10)out.conclusion="Reject unit-root null at 10% level (weak evidence of stationarity).";else out.conclusion="Do not reject unit-root null at the reported critical levels.";return out;
}

QString AnalysisEngine::number(double value) { return std::isfinite(value) ? QString::number(value,'f',4) : "—"; }
}
