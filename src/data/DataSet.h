#pragma once
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVariant>

namespace StatPro {
enum class VariableType { Numeric, String, Date, Boolean };

struct Variable {
    QString name;
    QString label;
    VariableType type{VariableType::String};
    QString format;
    QStringList valueLabels;
    QStringList missingValues;
};

class DataSet {
public:
    void clear();
    int rowCount() const;
    int columnCount() const;
    QStringList columnNames() const;
    const QVector<Variable>& variables() const;
    const QVariant& value(int row, int column) const;
    void setColumns(const QVector<Variable>&);
    void addRow(const QVector<QVariant>&);
    bool setValue(int row, int column, const QVariant&);
    bool setVariable(int column, const Variable&);
    bool addVariable(const Variable&, const QVariant& defaultValue = {});
    bool removeVariable(int column);
    bool renameVariable(int column, const QString&);
    bool saveCsv(const QString&, QString* error = nullptr) const;
    QVector<int> filteredRows(const QString& query) const;
private:
    QVector<Variable> m_variables;
    QVector<QVector<QVariant>> m_rows;
};
QString variableTypeName(VariableType type);
VariableType variableTypeFromName(const QString& name);
}
