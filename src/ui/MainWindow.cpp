#include "MainWindow.h"
#include "../data/CsvImporter.h"
#include "../data/ProjectManager.h"
#include <QApplication>
#include <QDockWidget>
#include <QTableWidget>
#include <QAction>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace StatPro {
MainWindow::MainWindow(QWidget* parent):QMainWindow(parent){resize(1550,920);setWindowTitle("StatPro Analytics");buildInterface();applyTheme();connect(&m_state,&AppState::dirtyChanged,this,&MainWindow::updateTitle);}
void MainWindow::buildInterface(){
 buildRibbon();auto* central=new QWidget;auto* layout=new QVBoxLayout(central);layout->setContentsMargins(8,6,8,6);
 m_projectTitle=new QLabel;m_projectTitle->setObjectName("ProjectTitle");layout->addWidget(m_projectTitle);
 auto* filterRow=new QHBoxLayout;m_filterEdit=new QLineEdit;m_filterEdit->setPlaceholderText("Filter rows…");
 auto* apply=new QPushButton("Apply");auto* clear=new QPushButton("Clear");auto* replace=new QPushButton("Find / Replace");
 filterRow->addWidget(new QLabel("Data filter:"));filterRow->addWidget(m_filterEdit,1);filterRow->addWidget(apply);filterRow->addWidget(clear);filterRow->addWidget(replace);layout->addLayout(filterRow);
 connect(apply,&QPushButton::clicked,this,&MainWindow::applyFilter);connect(clear,&QPushButton::clicked,this,&MainWindow::clearFilter);connect(replace,&QPushButton::clicked,this,&MainWindow::replaceText);
 m_tabs=new QTabWidget;m_grid=new QTableWidget;m_grid->setAlternatingRowColors(true);m_grid->setSortingEnabled(true);m_tabs->addTab(m_grid,"Data Editor");
 m_output=new QPlainTextEdit;m_output->setReadOnly(true);m_tabs->addTab(m_output,"Results / Output");layout->addWidget(m_tabs,1);setCentralWidget(central);
 buildVariablesPanel();buildPropertiesPanel();updateTitle();statusBar()->showMessage("Ready • Offline mode");
}
void MainWindow::buildRibbon(){
 auto* tb=addToolBar("StatPro Ribbon");tb->setMovable(false);tb->setToolButtonStyle(Qt::ToolButtonTextOnly);
 auto add=[&](const QString& n,auto fn){auto* a=tb->addAction(n);connect(a,&QAction::triggered,this,fn);};
 add("New",[this]{newProject();});add("Open",[this]{openProject();});add("Save",[this]{saveProject();});tb->addSeparator();
 add("Import CSV",[this]{importCsv();});add("Export CSV",[this]{exportCsv();});tb->addSeparator();
 add("Add Variable",[this]{addVariable();});add("Edit Variable",[this]{editVariable();});add("Delete Variable",[this]{deleteVariable();});tb->addSeparator();
 for(const auto& group:QStringList{"Data","Cleaning","Transform","Describe","Tests","Regression","Time Series","Econometrics","Survival","Survey","Multivariate","Machine Learning","Graphs","Diagnostics","Interpret","Reports"}){
  auto* a=tb->addAction(group);connect(a,&QAction::triggered,this,[this,group]{m_output->appendPlainText("\n["+group+"] module selected. Statistical procedures will be added in the analysis-engine phases.");m_tabs->setCurrentWidget(m_output);});
 }
 tb->addSeparator();add("Light / Dark",[this]{toggleTheme();});
}
void MainWindow::buildVariablesPanel(){
 auto* dock=new QDockWidget("Variables / Elements",this);m_variables=new QListWidget;dock->setWidget(m_variables);addDockWidget(Qt::LeftDockWidgetArea,dock);
 connect(m_variables,&QListWidget::currentRowChanged,this,[this](int r){if(r<0||r>=m_data.variables().size())return;const auto& v=m_data.variables()[r];m_properties->clear();m_properties->addTopLevelItem(new QTreeWidgetItem({"Name",v.name}));m_properties->addTopLevelItem(new QTreeWidgetItem({"Label",v.label}));m_properties->addTopLevelItem(new QTreeWidgetItem({"Type",variableTypeName(v.type)}));m_properties->addTopLevelItem(new QTreeWidgetItem({"Format",v.format}));m_properties->addTopLevelItem(new QTreeWidgetItem({"Missing values",v.missingValues.join(", ")}));m_properties->addTopLevelItem(new QTreeWidgetItem({"Observations",QString::number(m_data.rowCount())}));});
}
void MainWindow::buildPropertiesPanel(){auto* dock=new QDockWidget("Properties",this);m_properties=new QTreeWidget;m_properties->setHeaderLabels({"Property","Value"});dock->setWidget(m_properties);addDockWidget(Qt::RightDockWidgetArea,dock);}
void MainWindow::newProject(){if(!confirmSaveIfDirty())return;m_data.clear();m_state.setProjectPath("");m_state.setProjectName("Untitled Project");m_state.setDirty(false);refreshDataView();refreshVariables();m_output->setPlainText("New project created.");}
void MainWindow::openProject(){if(!confirmSaveIfDirty())return;const auto path=QFileDialog::getOpenFileName(this,"Open StatPro Project",{},"StatPro Project (*.stpro)");if(path.isEmpty())return;QString name,error;if(!ProjectManager::openProject(path,name,m_data,&error)){QMessageBox::critical(this,"Open Project",error);return;}m_state.setProjectPath(path);m_state.setProjectName(name);m_state.setDirty(false);m_state.addRecentProject(path);refreshDataView();refreshVariables();m_output->setPlainText("Project opened: "+path);}
void MainWindow::saveProject(){if(m_state.projectPath().isEmpty()){saveProjectAs();return;}QString error;if(!ProjectManager::saveProject(m_state.projectPath(),m_state.projectName(),m_data,&error))QMessageBox::critical(this,"Save Project",error);else{m_state.setDirty(false);m_state.addRecentProject(m_state.projectPath());statusBar()->showMessage("Project saved");}}
void MainWindow::saveProjectAs(){const auto path=QFileDialog::getSaveFileName(this,"Save StatPro Project",{},"StatPro Project (*.stpro)");if(path.isEmpty())return;m_state.setProjectPath(path);m_state.setProjectName(QFileInfo(path).completeBaseName());updateTitle();saveProject();}
void MainWindow::importCsv(){const auto path=QFileDialog::getOpenFileName(this,"Import CSV",{},"CSV files (*.csv);;All files (*.*)");if(path.isEmpty())return;QString error;if(!CsvImporter::importFile(path,m_data,&error)){QMessageBox::critical(this,"Import CSV",error);return;}m_state.setDirty(true);refreshDataView();refreshVariables();m_output->setPlainText("CSV imported successfully.\n"+path);m_tabs->setCurrentWidget(m_grid);}
void MainWindow::exportCsv(){const auto path=QFileDialog::getSaveFileName(this,"Export CSV",{},"CSV files (*.csv)");if(path.isEmpty())return;QString error;if(!m_data.saveCsv(path,&error))QMessageBox::critical(this,"Export CSV",error);else statusBar()->showMessage("CSV exported");}
void MainWindow::toggleTheme(){m_state.setDarkMode(!m_state.darkMode());applyTheme();}
void MainWindow::addVariable(){bool ok=false;const auto name=QInputDialog::getText(this,"Add Variable","Variable name:",QLineEdit::Normal,"",&ok);if(!ok||name.trimmed().isEmpty())return;Variable v;v.name=name.trimmed();if(!m_data.addVariable(v)){QMessageBox::warning(this,"Add Variable","A variable with that name already exists.");return;}m_state.setDirty(true);refreshDataView();refreshVariables();}
void MainWindow::editVariable(){int c=m_variables->currentRow();if(c<0)return;Variable v=m_data.variables()[c];bool ok=false;const auto name=QInputDialog::getText(this,"Edit Variable","Variable name:",QLineEdit::Normal,v.name,&ok);if(!ok)return;const auto label=QInputDialog::getText(this,"Edit Variable","Variable label:",QLineEdit::Normal,v.label,&ok);if(!ok)return;v.name=name.trimmed();v.label=label;if(!m_data.setVariable(c,v)){QMessageBox::warning(this,"Edit Variable","Invalid or duplicate variable name.");return;}m_state.setDirty(true);refreshDataView();refreshVariables();m_variables->setCurrentRow(c);}
void MainWindow::deleteVariable(){int c=m_variables->currentRow();if(c<0)return;if(QMessageBox::question(this,"Delete Variable","Delete the selected variable and its data?")==QMessageBox::Yes){m_data.removeVariable(c);m_state.setDirty(true);refreshDataView();refreshVariables();}}
void MainWindow::applyFilter(){const auto rows=m_data.filteredRows(m_filterEdit->text());m_grid->setSortingEnabled(false);m_grid->clear();m_grid->setRowCount(rows.size());m_grid->setColumnCount(m_data.columnCount());m_grid->setHorizontalHeaderLabels(m_data.columnNames());for(int rr=0;rr<rows.size();++rr)for(int c=0;c<m_data.columnCount();++c)m_grid->setItem(rr,c,new QTableWidgetItem(m_data.value(rows[rr],c).toString()));m_grid->setSortingEnabled(true);statusBar()->showMessage(QString("%1 rows shown").arg(rows.size()));}
void MainWindow::clearFilter(){m_filterEdit->clear();refreshDataView();}
void MainWindow::replaceText(){bool ok=false;const auto find=QInputDialog::getText(this,"Find / Replace","Find:",QLineEdit::Normal,"",&ok);if(!ok||find.isEmpty())return;const auto repl=QInputDialog::getText(this,"Find / Replace","Replace with:",QLineEdit::Normal,"",&ok);if(!ok)return;int count=0;for(int r=0;r<m_data.rowCount();++r)for(int c=0;c<m_data.columnCount();++c){auto s=m_data.value(r,c).toString();if(s.contains(find,Qt::CaseInsensitive)){s.replace(find,repl,Qt::CaseInsensitive);m_data.setValue(r,c,s);++count;}}if(count){m_state.setDirty(true);refreshDataView();}m_output->setPlainText(QString("Find / Replace complete. %1 cells changed.").arg(count));}
void MainWindow::refreshDataView(){m_grid->setSortingEnabled(false);m_grid->clear();m_grid->setRowCount(m_data.rowCount());m_grid->setColumnCount(m_data.columnCount());m_grid->setHorizontalHeaderLabels(m_data.columnNames());for(int r=0;r<m_data.rowCount();++r)for(int c=0;c<m_data.columnCount();++c)m_grid->setItem(r,c,new QTableWidgetItem(m_data.value(r,c).toString()));m_grid->setSortingEnabled(true);m_grid->resizeColumnsToContents();}
void MainWindow::refreshVariables(){m_variables->clear();for(const auto& v:m_data.variables())m_variables->addItem(v.name);m_properties->clear();}
void MainWindow::updateTitle(){setWindowTitle(QString("StatPro Analytics — %1%2").arg(m_state.projectName(),m_state.dirty()?" *":""));if(m_projectTitle)m_projectTitle->setText(m_state.projectName()+(m_state.dirty()?" *":""));}
bool MainWindow::confirmSaveIfDirty(){if(!m_state.dirty())return true;const auto ans=QMessageBox::warning(this,"Unsaved Changes","This project has unsaved changes.",QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel);if(ans==QMessageBox::Save){saveProject();return !m_state.dirty();}return ans==QMessageBox::Discard;}
void MainWindow::closeEvent(QCloseEvent* e){if(confirmSaveIfDirty())e->accept();else e->ignore();}
void MainWindow::applyTheme(){
 if(m_state.darkMode())setStyleSheet(R"(QMainWindow,QDockWidget,QTabWidget::pane{background:#20242a;color:#e8eaed}QToolBar{background:#292e36;border:1px solid #3b414b;padding:5px;spacing:4px}QToolButton{color:#e8eaed;padding:7px 9px;border-radius:5px}QToolButton:hover{background:#3a414c}QTableWidget,QListWidget,QTreeWidget,QPlainTextEdit,QLineEdit{background:#252a31;color:#e8eaed;border:1px solid #414752}QHeaderView::section{background:#303640;color:#e8eaed;padding:5px}QLabel#ProjectTitle{font-size:18px;font-weight:600;padding:3px})");
 else setStyleSheet(R"(QMainWindow,QDockWidget,QTabWidget::pane{background:#f4f6f8;color:#20242a}QToolBar{background:#ffffff;border:1px solid #d8dde3;padding:5px;spacing:4px}QToolButton{padding:7px 9px;border-radius:5px}QToolButton:hover{background:#e9eef5}QTableWidget,QListWidget,QTreeWidget,QPlainTextEdit,QLineEdit{background:#ffffff;color:#20242a;border:1px solid #d8dde3}QHeaderView::section{background:#eef1f4;padding:5px}QLabel#ProjectTitle{font-size:18px;font-weight:600;padding:3px})");
}
}
