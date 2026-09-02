#include "CsvImporter.h"
#include <QFile>
#include <QTextStream>
#include <QDate>
#include <QSet>

namespace StatPro {
static QStringList splitCsvLine(const QString& line){
    QStringList out; QString cur; bool quoted=false;
    for(int i=0;i<line.size();++i){QChar c=line[i];
        if(c=='"'){if(quoted&&i+1<line.size()&&line[i+1]=='"'){cur+='"';++i;}else quoted=!quoted;}
        else if(c==','&&!quoted){out<<cur;cur.clear();} else cur+=c;
    } out<<cur; return out;
}
static QString uniqueName(const QString& raw,int index,const QSet<QString>& used){
    QString base=raw.trimmed(); if(base.isEmpty()) base=QString("var_%1").arg(index+1);
    QString name=base; int n=2; auto exists=[&](const QString& candidate){for(const auto& u:used)if(u.compare(candidate,Qt::CaseInsensitive)==0)return true;return false;}; while(exists(name)) name=base+QString("_%1").arg(n++); return name;
}
static VariableType inferType(const QStringList& values){
    bool numeric=true,date=true,boolean=true; bool saw=false;
    for(const auto& raw:values){QString s=raw.trimmed();if(s.isEmpty())continue;saw=true;bool ok=false;s.toDouble(&ok);numeric &= ok;date &= QDate::fromString(s,Qt::ISODate).isValid();boolean &= (s.compare("true",Qt::CaseInsensitive)==0||s.compare("false",Qt::CaseInsensitive)==0||s=="0"||s=="1");}
    if(!saw)return VariableType::String;if(numeric)return VariableType::Numeric;if(date)return VariableType::Date;if(boolean)return VariableType::Boolean;return VariableType::String;
}
bool CsvImporter::importFile(const QString& path,DataSet& target,QString* error){
    QFile f(path); if(!f.open(QIODevice::ReadOnly|QIODevice::Text)){if(error)*error=f.errorString();return false;}
    QTextStream in(&f); if(in.atEnd()){if(error)*error="The CSV file is empty.";return false;}
    const auto names=splitCsvLine(in.readLine()); QVector<QStringList> records; while(!in.atEnd()){auto cells=splitCsvLine(in.readLine());cells.resize(names.size());records.push_back(cells);}
    QVector<Variable> vars; QSet<QString> used;
    for(int i=0;i<names.size();++i){Variable v;v.name=uniqueName(names.value(i),i,used);used.insert(v.name);QStringList vals;for(const auto& row:records)vals<<row.value(i);v.type=inferType(vals);vars.push_back(v);}
    DataSet d;if(!d.setColumns(vars)){if(error)*error="The CSV contains invalid or duplicate column names.";return false;}
    for(const auto& cells:records){QVector<QVariant> row;for(int c=0;c<vars.size();++c)row<<cells.value(c);if(!d.appendRow(row)){if(error)*error=QString("A value in imported row %1 does not match its inferred variable type.").arg(d.rowCount()+1);return false;}}
    target=d;return true;
}
}
