#include "DataSet.h"
#include <QDate>
#include <QFile>
#include <QTextStream>
#include <QSet>
#include <QRegularExpression>
#include <algorithm>

namespace StatPro {
void DataSet::clear() { m_variables.clear(); m_rows.clear(); }
int DataSet::rowCount() const { return m_rows.size(); }
int DataSet::columnCount() const { return m_variables.size(); }
QStringList DataSet::columnNames() const { QStringList out; for (const auto& v : m_variables) out << v.name; return out; }
const QVector<Variable>& DataSet::variables() const { return m_variables; }
const QVariant& DataSet::value(int r, int c) const { return m_rows.at(r).at(c); }

bool DataSet::setColumns(const QVector<Variable>& columns) {
    QSet<QString> names;
    for (const auto& v : columns) {
        if (v.name.trimmed().isEmpty() || names.contains(v.name)) return false;
        names.insert(v.name);
    }
    m_variables = columns;
    for (auto& row : m_rows) row.resize(m_variables.size());
    return true;
}

bool DataSet::validateValue(int column, const QVariant& value, QString* error) const {
    if (column < 0 || column >= columnCount()) return false;
    const QString text = value.toString().trimmed();
    if (text.isEmpty()) return true;
    const auto type = m_variables[column].type;
    bool ok = true;
    switch (type) {
    case VariableType::Numeric:
        text.toDouble(&ok);
        if (!ok && error) *error = QString("'%1' is not a valid numeric value.").arg(text);
        break;
    case VariableType::Date:
        ok = QDate::fromString(text, Qt::ISODate).isValid() || QDate::fromString(text, "yyyy-MM-dd").isValid();
        if (!ok && error) *error = QString("'%1' is not a valid date. Use YYYY-MM-DD.").arg(text);
        break;
    case VariableType::Boolean:
        ok = text.compare("true", Qt::CaseInsensitive) == 0 || text.compare("false", Qt::CaseInsensitive) == 0 || text == "0" || text == "1";
        if (!ok && error) *error = QString("'%1' is not a valid Boolean value. Use true/false or 1/0.").arg(text);
        break;
    case VariableType::String:
        break;
    }
    return ok;
}

bool DataSet::setValue(int r, int c, const QVariant& v) {
    if (r < 0 || r >= rowCount() || c < 0 || c >= columnCount()) return false;
    if (!validateValue(c, v)) return false;
    m_rows[r][c] = v;
    return true;
}

bool DataSet::setVariable(int c, const Variable& v, QString* error) {
    if (c < 0 || c >= columnCount()) {
        if (error) *error = "Invalid variable index.";
        return false;
    }
    if (v.name.trimmed().isEmpty()) {
        if (error) *error = "Variable name cannot be empty.";
        return false;
    }
    for (int i = 0; i < columnCount(); ++i) {
        if (i != c && m_variables[i].name.compare(v.name, Qt::CaseInsensitive) == 0) {
            if (error) *error = QString("The variable name '%1' is already in use.").arg(v.name);
            return false;
        }
    }
    for (int r = 0; r < rowCount(); ++r) {
        const QString text = m_rows[r][c].toString().trimmed();
        if (text.isEmpty()) continue;
        QString valueError;
        // Validate against the NEW variable definition, not the old one.
        switch (v.type) {
        case VariableType::Numeric: {
            bool ok=false; text.toDouble(&ok);
            if (!ok) valueError = QString("'%1' is not a valid numeric value.").arg(text);
            break;
        }
        case VariableType::Date:
            if (!(QDate::fromString(text, Qt::ISODate).isValid() || QDate::fromString(text, "yyyy-MM-dd").isValid()))
                valueError = QString("'%1' is not a valid date. Use YYYY-MM-DD.").arg(text);
            break;
        case VariableType::Boolean:
            if (!(text.compare("true", Qt::CaseInsensitive) == 0 || text.compare("false", Qt::CaseInsensitive) == 0 || text == "0" || text == "1"))
                valueError = QString("'%1' is not a valid Boolean value. Use true/false or 1/0.").arg(text);
            break;
        case VariableType::String:
            break;
        }
        if (!valueError.isEmpty()) {
            if (error) *error = QString("Row %1: %2").arg(r + 1).arg(valueError);
            return false;
        }
    }
    m_variables[c] = v;
    return true;
}

bool DataSet::addVariable(const Variable& v, const QVariant& def) {
    if (v.name.trimmed().isEmpty()) return false;
    for (const auto& x : m_variables) if (x.name.compare(v.name, Qt::CaseInsensitive) == 0) return false;
    m_variables.push_back(v);
    if (!validateValue(m_variables.size()-1, def)) { m_variables.removeLast(); return false; }
    for (auto& row : m_rows) row.push_back(def);
    return true;
}

bool DataSet::removeVariable(int c) {
    if (c < 0 || c >= columnCount()) return false;
    m_variables.removeAt(c);
    for (auto& row : m_rows) row.removeAt(c);
    return true;
}

bool DataSet::renameVariable(int c, const QString& name) {
    if (c < 0 || c >= columnCount() || name.trimmed().isEmpty()) return false;
    for (int i = 0; i < columnCount(); ++i) if (i != c && m_variables[i].name.compare(name, Qt::CaseInsensitive) == 0) return false;
    m_variables[c].name = name.trimmed();
    return true;
}

bool DataSet::insertRow(int rowIndex, const QVector<QVariant>& values) {
    if (rowIndex < 0 || rowIndex > rowCount()) return false;
    QVector<QVariant> r = values; r.resize(columnCount());
    for (int c = 0; c < columnCount(); ++c) if (!validateValue(c, r[c])) return false;
    m_rows.insert(rowIndex, r);
    return true;
}

bool DataSet::appendRow(const QVector<QVariant>& values) { return insertRow(rowCount(), values); }
bool DataSet::removeRow(int rowIndex) { if (rowIndex < 0 || rowIndex >= rowCount()) return false; m_rows.removeAt(rowIndex); return true; }
bool DataSet::setRow(int rowIndex, const QVector<QVariant>& values) { if (rowIndex < 0 || rowIndex >= rowCount()) return false; QVector<QVariant> r=values; r.resize(columnCount()); for(int c=0;c<columnCount();++c) if(!validateValue(c,r[c])) return false; m_rows[rowIndex]=r; return true; }
QVector<QVariant> DataSet::row(int rowIndex) const { return m_rows.at(rowIndex); }

bool DataSet::sortByColumn(int column, bool ascending) {
    if (column < 0 || column >= columnCount()) return false;
    std::stable_sort(m_rows.begin(), m_rows.end(), [column, ascending](const auto& a, const auto& b) {
        const QString av = a[column].toString();
        const QString bv = b[column].toString();
        bool aNum=false,bNum=false;
        const double ad=av.toDouble(&aNum), bd=bv.toDouble(&bNum);
        if (aNum && bNum) {
            if (ad == bd) return false;
            return ascending ? ad < bd : ad > bd;
        }
        const int cmp = QString::localeAwareCompare(av, bv);
        if (cmp == 0) return false;
        return ascending ? cmp < 0 : cmp > 0;
    });
    return true;
}

bool DataSet::isMissing(int r, int c) const {
    if (r < 0 || r >= rowCount() || c < 0 || c >= columnCount()) return true;
    const QString s=value(r,c).toString();
    return s.trimmed().isEmpty() || m_variables[c].missingValues.contains(s);
}
int DataSet::missingCount(int c) const { int n=0; for(int r=0;r<rowCount();++r) if(isMissing(r,c)) ++n; return n; }

static QString csvEscape(const QString& s) { QString out=s; out.replace("\"","\"\""); if(out.contains(',')||out.contains('"')||out.contains('\n')) return '"'+out+'"'; return out; }
bool DataSet::saveCsv(const QString& path, QString* error) const { QFile f(path); if(!f.open(QIODevice::WriteOnly|QIODevice::Text)){if(error)*error=f.errorString();return false;} QTextStream out(&f); out<<columnNames().join(',')<<'\n'; for(const auto& row:m_rows){QStringList cells;for(const auto& cell:row)cells<<csvEscape(cell.toString());out<<cells.join(',')<<'\n';} return true; }
QVector<int> DataSet::filteredRows(const QString& query) const {
    QVector<int> result; const QString q=query.trimmed();
    if(q.isEmpty()){for(int r=0;r<rowCount();++r)result<<r;return result;}
    QRegularExpression re(R"(^\s*([A-Za-z_][A-Za-z0-9_]*)\s*(==|!=|>=|<=|>|<)\s*(.*?)\s*$)");
    const auto match=re.match(q);
    if(match.hasMatch()){
        const QString name=match.captured(1); int c=-1; for(int i=0;i<columnCount();++i)if(m_variables[i].name.compare(name,Qt::CaseInsensitive)==0){c=i;break;}
        if(c>=0){const QString op=match.captured(2), rhs=match.captured(3).trimmed(); bool rhsNum=false; const double rd=rhs.toDouble(&rhsNum);
            for(int r=0;r<rowCount();++r){const QString lhs=value(r,c).toString().trimmed(); bool lhsNum=false; const double ld=lhs.toDouble(&lhsNum); bool pass=false;
                if(op=="==") pass=lhs.compare(rhs,Qt::CaseInsensitive)==0 || (lhsNum&&rhsNum&&ld==rd);
                else if(op=="!=") pass=!(lhs.compare(rhs,Qt::CaseInsensitive)==0 || (lhsNum&&rhsNum&&ld==rd));
                else if(lhsNum&&rhsNum){if(op==">")pass=ld>rd;else if(op=="<")pass=ld<rd;else if(op==">=")pass=ld>=rd;else if(op=="<=")pass=ld<=rd;}
                if(pass)result<<r;
            }
            return result;
        }
    }
    for(int r=0;r<rowCount();++r){for(int c=0;c<columnCount();++c)if(value(r,c).toString().contains(q,Qt::CaseInsensitive)){result<<r;break;}}
    return result;
}
QString variableTypeName(VariableType t){switch(t){case VariableType::Numeric:return "Numeric";case VariableType::Date:return "Date";case VariableType::Boolean:return "Boolean";default:return "String";}}
VariableType variableTypeFromName(const QString& n){if(n.compare("Numeric",Qt::CaseInsensitive)==0)return VariableType::Numeric;if(n.compare("Date",Qt::CaseInsensitive)==0)return VariableType::Date;if(n.compare("Boolean",Qt::CaseInsensitive)==0)return VariableType::Boolean;return VariableType::String;}
}
