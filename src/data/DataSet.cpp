#include "DataSet.h"
#include <QFile>
#include <QTextStream>
namespace StatPro {
void DataSet::clear() { m_variables.clear(); m_rows.clear(); }
int DataSet::rowCount() const { return m_rows.size(); }
int DataSet::columnCount() const { return m_variables.size(); }
QStringList DataSet::columnNames() const {
    QStringList out; for (const auto& v : m_variables) out << v.name; return out;
}
const QVector<Variable>& DataSet::variables() const { return m_variables; }
const QVariant& DataSet::value(int r,int c) const { return m_rows.at(r).at(c); }
void DataSet::setColumns(const QVector<Variable>& v) {
    m_variables=v; for(auto& row:m_rows) row.resize(m_variables.size());
}
void DataSet::addRow(const QVector<QVariant>& row) {
    auto r=row; r.resize(m_variables.size()); m_rows.push_back(r);
}
bool DataSet::setValue(int r,int c,const QVariant& v) {
    if(r<0||r>=rowCount()||c<0||c>=columnCount()) return false;
    m_rows[r][c]=v; return true;
}
bool DataSet::setVariable(int c,const Variable& v) {
    if(c<0||c>=columnCount()) return false; m_variables[c]=v; return true;
}
bool DataSet::addVariable(const Variable& v,const QVariant& def) {
    if(v.name.isEmpty()||columnNames().contains(v.name)) return false;
    m_variables.push_back(v); for(auto& row:m_rows) row.push_back(def); return true;
}
bool DataSet::removeVariable(int c) {
    if(c<0||c>=columnCount()) return false;
    m_variables.removeAt(c); for(auto& row:m_rows) row.removeAt(c); return true;
}
bool DataSet::renameVariable(int c,const QString& name) {
    if(c<0||c>=columnCount()||name.isEmpty()||columnNames().contains(name)) return false;
    m_variables[c].name=name; return true;
}
static QString csvEscape(const QString& s) {
    QString out=s; out.replace("\"","\"\"");
    if(out.contains(',')||out.contains('"')||out.contains('\n')) return "\"" + out + "\"";
    return out;
}
bool DataSet::saveCsv(const QString& path,QString* error) const {
    QFile f(path);
    if(!f.open(QIODevice::WriteOnly|QIODevice::Text)){if(error)*error=f.errorString();return false;}
    QTextStream out(&f); out << columnNames().join(',') << '\n';
    for(const auto& row:m_rows){QStringList cells;for(const auto& cell:row)cells<<csvEscape(cell.toString());out<<cells.join(',')<<'\n';}
    return true;
}
QVector<int> DataSet::filteredRows(const QString& query) const {
    QVector<int> result; const QString q=query.trimmed();
    for(int r=0;r<rowCount();++r){
        if(q.isEmpty()){result.push_back(r);continue;}
        for(int c=0;c<columnCount();++c)
            if(value(r,c).toString().contains(q,Qt::CaseInsensitive)){result.push_back(r);break;}
    }
    return result;
}
QString variableTypeName(VariableType t){
    switch(t){case VariableType::Numeric:return "Numeric";case VariableType::Date:return "Date";case VariableType::Boolean:return "Boolean";default:return "String";}
}
VariableType variableTypeFromName(const QString& n){
    if(n.compare("Numeric",Qt::CaseInsensitive)==0)return VariableType::Numeric;
    if(n.compare("Date",Qt::CaseInsensitive)==0)return VariableType::Date;
    if(n.compare("Boolean",Qt::CaseInsensitive)==0)return VariableType::Boolean;
    return VariableType::String;
}
}
