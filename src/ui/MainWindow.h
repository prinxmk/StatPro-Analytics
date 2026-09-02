#pragma once
#include <QMainWindow>
#include "../core/AppState.h"
#include "../data/DataSet.h"
class QTableWidget;class QListWidget;class QTreeWidget;class QLabel;class QTabWidget;class QPlainTextEdit;class QLineEdit;
namespace StatPro {
class MainWindow:public QMainWindow{
 Q_OBJECT
public: explicit MainWindow(QWidget* parent=nullptr);
protected:void closeEvent(QCloseEvent*)override;
private slots:
 void newProject();void openProject();void saveProject();void saveProjectAs();void importCsv();void exportCsv();void toggleTheme();
 void addVariable();void editVariable();void deleteVariable();void applyFilter();void clearFilter();void replaceText();
private:
 void buildInterface();void buildRibbon();void buildVariablesPanel();void buildPropertiesPanel();void refreshDataView();void refreshVariables();void updateTitle();void applyTheme();
 bool confirmSaveIfDirty();
 AppState m_state;DataSet m_data;QTabWidget* m_tabs{};QTableWidget* m_grid{};QListWidget* m_variables{};QTreeWidget* m_properties{};QLabel* m_projectTitle{};QPlainTextEdit* m_output{};QLineEdit* m_filterEdit{};
};
}
