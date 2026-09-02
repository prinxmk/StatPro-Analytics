#pragma once
#include "DataSet.h"
namespace StatPro {
class ProjectManager {
public:
 static bool saveProject(const QString&,const QString&,const DataSet&,QString* error=nullptr);
 static bool openProject(const QString&,QString&,DataSet&,QString* error=nullptr);
};
}
