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
    bool setValue(int row, int column, const QVariant& value);
    bool setColumns(const QVector<Variable>& columns);
    bool setVariable(int column, const Variable& variable, QString* error = nullptr);
    bool addVariable(const Variable& variable, const QVariant& defaultValue = {});
    bool removeVariable(int column);
    bool renameVariable(int column, const QString& name);
    bool insertRow(int row, const QVector<QVariant>& values = {});
    bool appendRow(const QVector<QVariant>& values = {});
    bool removeRow(int row);
    bool setRow(int row, const QVector<QVariant>& values);
    QVector<QVariant> row(int row) const;
    bool sortByColumn(int column, bool ascending = true);
    bool validateValue(int column, const QVariant& value, QString* error = nullptr) const;
    bool isMissing(int row, int column) const;
    int missingCount(int column) const;
    bool saveCsv(const QString&, QString* error = nullptr) const;
    bool saveDelimited(const QString&, QChar delimiter, QString* error = nullptr) const;
    QVector<int> filteredRows(const QString& query) const;

private:
    QVector<Variable> m_variables;
    QVector<QVector<QVariant>> m_rows;
};

QString variableTypeName(VariableType type);
VariableType variableTypeFromName(const QString& name);
}
