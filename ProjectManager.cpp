#include "ProjectManager.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
namespace StatPro {
bool ProjectManager::saveProject(const QString& path,const QString& name,const DataSet& data,QString* error){
    QJsonObject root;root["format"]="StatPro Project";root["version"]=2;root["projectName"]=name;QJsonArray vars;
    for(const auto& v:data.variables()){QJsonObject o;o["name"]=v.name;o["label"]=v.label;o["type"]=variableTypeName(v.type);o["format"]=v.format;o["valueLabels"]=QJsonArray::fromStringList(v.valueLabels);o["missingValues"]=QJsonArray::fromStringList(v.missingValues);vars.append(o);}
    root["variables"]=vars;QJsonArray rows;
    for(int r=0;r<data.rowCount();++r){QJsonArray row;for(int c=0;c<data.columnCount();++c)row.append(data.value(r,c).toString());rows.append(row);}
    root["data"]=rows;QFile f(path);if(!f.open(QIODevice::WriteOnly)){if(error)*error=f.errorString();return false;}f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));return true;
}
bool ProjectManager::openProject(const QString& path,QString& name,DataSet& data,QString* error){
    QFile f(path);if(!f.open(QIODevice::ReadOnly)){if(error)*error=f.errorString();return false;}
    QJsonParseError pe;auto doc=QJsonDocument::fromJson(f.readAll(),&pe);if(pe.error!=QJsonParseError::NoError||!doc.isObject()){if(error)*error="Invalid StatPro project.";return false;}
    auto root=doc.object();name=root["projectName"].toString("Untitled Project");QVector<Variable> vars;
    for(const auto& item:root["variables"].toArray()){auto o=item.toObject();Variable v;v.name=o["name"].toString();v.label=o["label"].toString();v.type=variableTypeFromName(o["type"].toString());v.format=o["format"].toString();for(const auto& x:o["valueLabels"].toArray())v.valueLabels<<x.toString();for(const auto& x:o["missingValues"].toArray())v.missingValues<<x.toString();vars.push_back(v);}
    DataSet d;d.setColumns(vars);for(const auto& item:root["data"].toArray()){QVector<QVariant> row;for(const auto& x:item.toArray())row<<x.toString();d.appendRow(row);}data=d;return true;
}
}
