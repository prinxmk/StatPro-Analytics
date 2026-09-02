#pragma once
#include "DataSet.h"
namespace StatPro {
class CsvImporter { public: static bool importFile(const QString&, DataSet&, QString* error=nullptr); };
}
