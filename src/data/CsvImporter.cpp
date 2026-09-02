#include "CsvImporter.h"
#include <QFile>
#include <QTextStream>
namespace StatPro {
static QStringList splitCsvLine(const QString& line){
    QStringList out; QString cur; bool quoted=false;
    for(int i=0;i<line.size();++i){QChar c=line[i];
        if(c=='"'){if(quoted&&i+1<line.size()&&line[i+1]=='"'){cur+='"';++i;}else quoted=!quoted;}
        else if(c==','&&!quoted){out<<cur;cur.clear();} else cur+=c;
    } out<<cur; return out;
}
bool CsvImporter::importFile(const QString& path,DataSet& target,QString* error){
    QFile f(path); if(!f.open(QIODevice::ReadOnly|QIODevice::Text)){if(error)*error=f.errorString();return false;}
    QTextStream in(&f); if(in.atEnd()){if(error)*error="The CSV file is empty.";return false;}
    const auto names=splitCsvLine(in.readLine()); QVector<Variable> vars;
    for(int i=0;i<names.size();++i){Variable v;v.name=names[i].trimmed();if(v.name.isEmpty())v.name=QString("var_%1").arg(i+1);vars.push_back(v);}
    DataSet d;d.setColumns(vars);
    while(!in.atEnd()){auto cells=splitCsvLine(in.readLine());QVector<QVariant> row;for(const auto& c:cells)row<<c;d.addRow(row);}
    target=d;return true;
}
}
