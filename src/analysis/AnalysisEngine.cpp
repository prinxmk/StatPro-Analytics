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

QString AnalysisEngine::number(double value) { return std::isfinite(value) ? QString::number(value,'f',4) : "—"; }
}
