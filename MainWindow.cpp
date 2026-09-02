#include "MainWindow.h"
#include "../data/CsvImporter.h"
#include "../data/ProjectManager.h"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDockWidget>
#include <QSet>
#include <QComboBox>
#include <QDate>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStatusBar>
#include <QTableWidget>
#include <QTabWidget>
#include <QToolBar>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QCheckBox>
#include <QUndoCommand>
#include <algorithm>

namespace StatPro {

class CellEditCommand : public QUndoCommand {
public:
    CellEditCommand(MainWindow* w,int r,int c,const QString& oldV,const QString& newV):m_w(w),m_r(r),m_c(c),m_old(oldV),m_new(newV){setText("Edit cell");}
    void undo() override {m_w->setCellFromText(m_r,m_c,m_old);}
    void redo() override {m_w->setCellFromText(m_r,m_c,m_new);}
private: MainWindow* m_w; int m_r,m_c; QString m_old,m_new;
};

MainWindow::MainWindow(QWidget* parent):QMainWindow(parent),m_undoStack(new QUndoStack(this)){
    resize(1550,920); setWindowTitle("StatPro Analytics"); buildInterface(); applyTheme();
    connect(&m_state,&AppState::dirtyChanged,this,&MainWindow::updateTitle);
}

void MainWindow::buildInterface(){
    buildRibbon();
    auto* central=new QWidget; auto* layout=new QVBoxLayout(central); layout->setContentsMargins(8,6,8,6); layout->setSpacing(6);
    m_projectTitle=new QLabel; m_projectTitle->setObjectName("ProjectTitle"); layout->addWidget(m_projectTitle);
    auto* filterRow=new QHBoxLayout; m_filterEdit=new QLineEdit; m_filterEdit->setPlaceholderText("Filter rows… (text search or: age > 30, sex == Male)");
    auto* apply=new QPushButton("Apply"); auto* clear=new QPushButton("Clear"); auto* replace=new QPushButton("Find / Replace");
    filterRow->addWidget(new QLabel("Data filter:")); filterRow->addWidget(m_filterEdit,1); filterRow->addWidget(apply); filterRow->addWidget(clear); filterRow->addWidget(replace); layout->addLayout(filterRow);
    connect(apply,&QPushButton::clicked,this,&MainWindow::applyFilter); connect(clear,&QPushButton::clicked,this,&MainWindow::clearFilter); connect(replace,&QPushButton::clicked,this,&MainWindow::replaceText);
    m_tabs=new QTabWidget; m_grid=new QTableWidget; m_grid->setAlternatingRowColors(true); m_grid->setSelectionMode(QAbstractItemView::ExtendedSelection); m_grid->setSelectionBehavior(QAbstractItemView::SelectItems); m_grid->setEditTriggers(QAbstractItemView::DoubleClicked|QAbstractItemView::EditKeyPressed|QAbstractItemView::AnyKeyPressed); m_grid->setSortingEnabled(false); m_grid->verticalHeader()->setDefaultSectionSize(24); m_grid->horizontalHeader()->setSectionsMovable(false); m_grid->setCornerButtonEnabled(true);
    m_tabs->addTab(m_grid,"Data Editor");
    connect(m_grid,&QTableWidget::cellChanged,this,&MainWindow::cellChanged);
    m_output=new QPlainTextEdit; m_output->setReadOnly(true); m_tabs->addTab(m_output,"Results / Output"); layout->addWidget(m_tabs,1); setCentralWidget(central);
    buildVariablesPanel(); buildPropertiesPanel(); updateTitle(); updateStatus();
}

void MainWindow::buildRibbon(){
    auto* tb=addToolBar("StatPro Ribbon"); tb->setMovable(false); tb->setToolButtonStyle(Qt::ToolButtonTextOnly);
    auto add=[&](const QString& n,auto fn){auto* a=tb->addAction(n);connect(a,&QAction::triggered,this,fn);};
    add("New",[this]{newProject();}); add("Open",[this]{openProject();}); add("Save",[this]{saveProject();}); tb->addSeparator();
    add("Import CSV",[this]{importCsv();}); add("Export CSV",[this]{exportCsv();}); tb->addSeparator();
    add("Add Variable",[this]{addVariable();}); add("Edit Variable",[this]{editVariable();}); add("Delete Variable",[this]{deleteVariable();}); tb->addSeparator();
    add("Add Row",[this]{addRow();}); add("Insert Row",[this]{insertRow();}); add("Delete Row(s)",[this]{deleteRows();}); tb->addSeparator();
    add("Copy",[this]{copySelection();}); add("Paste",[this]{pasteSelection();}); add("Undo",[this]{undo();}); add("Redo",[this]{redo();}); tb->addSeparator();
    add("Sort ↑",[this]{sortAscending();}); add("Sort ↓",[this]{sortDescending();}); add("Dataset Info",[this]{showDatasetInfo();}); tb->addSeparator();
    for(const auto& group:QStringList{"Data","Cleaning","Transform","Describe","Tests","Regression","Time Series","Econometrics","Survival","Survey","Multivariate","Machine Learning","Graphs","Diagnostics","Interpret","Reports"}){auto* a=tb->addAction(group);connect(a,&QAction::triggered,this,[this,group]{m_output->appendPlainText("\n["+group+"] module selected. Statistical procedures will be added in the analysis-engine phases.");m_tabs->setCurrentWidget(m_output);});}
    tb->addSeparator(); add("Light / Dark",[this]{toggleTheme();});
}

void MainWindow::buildVariablesPanel(){
    auto* dock=new QDockWidget("Variables / Elements",this); m_variables=new QListWidget; m_variables->setAlternatingRowColors(true); dock->setWidget(m_variables); addDockWidget(Qt::LeftDockWidgetArea,dock);
    connect(m_variables,&QListWidget::currentRowChanged,this,[this](int r){refreshProperties(r);});
}
void MainWindow::buildPropertiesPanel(){auto* dock=new QDockWidget("Properties",this);m_properties=new QTreeWidget;m_properties->setHeaderLabels({"Property","Value"});m_properties->setAlternatingRowColors(true);dock->setWidget(m_properties);addDockWidget(Qt::RightDockWidgetArea,dock);}

void MainWindow::newProject(){if(!confirmSaveIfDirty())return;m_data.clear();m_state.setProjectPath("");m_state.setProjectName("Untitled Project");m_state.setDirty(false);m_undoStack->clear();m_filterEdit->clear();refreshDataView();refreshVariables();m_output->setPlainText("New project created.");}
void MainWindow::openProject(){if(!confirmSaveIfDirty())return;const auto path=QFileDialog::getOpenFileName(this,"Open StatPro Project",{},"StatPro Project (*.stpro)");if(path.isEmpty())return;QString name,error;if(!ProjectManager::openProject(path,name,m_data,&error)){QMessageBox::critical(this,"Open Project",error);return;}m_state.setProjectPath(path);m_state.setProjectName(name);m_state.setDirty(false);m_state.addRecentProject(path);m_undoStack->clear();m_filterEdit->clear();refreshDataView();refreshVariables();m_output->setPlainText("Project opened: "+path);}
void MainWindow::saveProject(){if(m_state.projectPath().isEmpty()){saveProjectAs();return;}QString error;if(!ProjectManager::saveProject(m_state.projectPath(),m_state.projectName(),m_data,&error))QMessageBox::critical(this,"Save Project",error);else{m_state.setDirty(false);m_state.addRecentProject(m_state.projectPath());statusBar()->showMessage("Project saved");}}
void MainWindow::saveProjectAs(){auto path=QFileDialog::getSaveFileName(this,"Save StatPro Project",{},"StatPro Project (*.stpro)");if(path.isEmpty())return;if(!path.endsWith(".stpro",Qt::CaseInsensitive))path += ".stpro";m_state.setProjectPath(path);m_state.setProjectName(QFileInfo(path).completeBaseName());updateTitle();saveProject();}
void MainWindow::importCsv(){const auto path=QFileDialog::getOpenFileName(this,"Import CSV",{},"CSV files (*.csv);;All files (*.*)");if(path.isEmpty())return;QString error;if(!CsvImporter::importFile(path,m_data,&error)){QMessageBox::critical(this,"Import CSV",error);return;}m_state.setDirty(true);m_undoStack->clear();refreshDataView();refreshVariables();m_output->setPlainText("CSV imported successfully.\n"+path);m_tabs->setCurrentWidget(m_grid);}
void MainWindow::exportCsv(){const auto path=QFileDialog::getSaveFileName(this,"Export CSV",{},"CSV files (*.csv)");if(path.isEmpty())return;QString error;if(!m_data.saveCsv(path,&error))QMessageBox::critical(this,"Export CSV",error);else statusBar()->showMessage("CSV exported");}
void MainWindow::toggleTheme(){m_state.setDarkMode(!m_state.darkMode());applyTheme();}

void MainWindow::addVariable(){
    QDialog d(this); d.setWindowTitle("Add Variable"); auto* form=new QFormLayout(&d); auto* name=new QLineEdit; auto* label=new QLineEdit; auto* type=new QComboBox; type->addItems({"String","Numeric","Date","Boolean"}); auto* missing=new QLineEdit; missing->setPlaceholderText("e.g. 99, -99");
    form->addRow("Name *",name);form->addRow("Label",label);form->addRow("Type",type);form->addRow("Missing values",missing);auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form->addRow(buttons);connect(buttons,&QDialogButtonBox::accepted,&d,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);if(d.exec()!=QDialog::Accepted)return;
    Variable v;v.name=name->text().trimmed();v.label=label->text().trimmed();v.type=variableTypeFromName(type->currentText());v.missingValues=missing->text().split(',',Qt::SkipEmptyParts);for(auto& x:v.missingValues)x=x.trimmed();if(v.name.isEmpty()||!m_data.addVariable(v)){QMessageBox::warning(this,"Add Variable","The variable name is empty or already exists.");return;}m_state.setDirty(true);refreshDataView();refreshVariables();m_variables->setCurrentRow(m_data.columnCount()-1);
}

void MainWindow::editVariable(){
    const int c=m_variables->currentRow(); if(c<0)return; Variable v=m_data.variables()[c]; QDialog d(this);d.setWindowTitle("Variable Editor");auto* form=new QFormLayout(&d);auto* name=new QLineEdit(v.name);auto* label=new QLineEdit(v.label);auto* type=new QComboBox;type->addItems({"String","Numeric","Date","Boolean"});type->setCurrentText(variableTypeName(v.type));auto* format=new QLineEdit(v.format);auto* missing=new QLineEdit(v.missingValues.join(", "));form->addRow("Name *",name);form->addRow("Label",label);form->addRow("Type",type);form->addRow("Format",format);form->addRow("Missing values",missing);auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form->addRow(buttons);connect(buttons,&QDialogButtonBox::accepted,&d,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);if(d.exec()!=QDialog::Accepted)return;
    v.name=name->text().trimmed();v.label=label->text().trimmed();v.type=variableTypeFromName(type->currentText());v.format=format->text().trimmed();v.missingValues=missing->text().split(',',Qt::SkipEmptyParts);for(auto& x:v.missingValues)x=x.trimmed();if(!m_data.setVariable(c,v)){QMessageBox::warning(this,"Variable Editor","Invalid or duplicate variable name.");return;}m_state.setDirty(true);refreshDataView();refreshVariables();m_variables->setCurrentRow(c);
}
void MainWindow::deleteVariable(){int c=m_variables->currentRow();if(c<0)return;if(QMessageBox::question(this,"Delete Variable","Delete the selected variable and its data?")==QMessageBox::Yes){m_data.removeVariable(c);m_state.setDirty(true);refreshDataView();refreshVariables();}}

QVector<int> MainWindow::visibleRows() const { QVector<int> rows; if (m_filterEdit->text().trimmed().isEmpty()) { for (int i=0;i<m_data.rowCount();++i) rows << i; } else rows = m_data.filteredRows(m_filterEdit->text()); return rows; }
int MainWindow::dataRowForViewRow(int viewRow) const{return viewRow>=0&&viewRow<m_visibleRows.size()?m_visibleRows[viewRow]:-1;}
QString MainWindow::displayValue(int r,int c) const{return m_data.value(r,c).toString();}

void MainWindow::refreshDataView(){refreshDataView(visibleRows());}
void MainWindow::refreshDataView(const QVector<int>& rows){
    m_refreshing=true;m_visibleRows=rows;m_grid->setUpdatesEnabled(false);m_grid->clear();m_grid->setRowCount(rows.size());m_grid->setColumnCount(m_data.columnCount());m_grid->setHorizontalHeaderLabels(m_data.columnNames());
    for(int rr=0;rr<rows.size();++rr){m_grid->setVerticalHeaderItem(rr,new QTableWidgetItem(QString::number(rows[rr]+1)));for(int c=0;c<m_data.columnCount();++c){auto* item=new QTableWidgetItem(displayValue(rows[rr],c));if(m_data.isMissing(rows[rr],c))item->setToolTip("Missing value");m_grid->setItem(rr,c,item);}}
    m_grid->setUpdatesEnabled(true);m_refreshing=false;m_grid->resizeColumnsToContents();updateStatus();
}
void MainWindow::refreshVariables(){m_variables->clear();for(const auto& v:m_data.variables()){auto* item=new QListWidgetItem(v.name);if(!v.label.isEmpty())item->setToolTip(v.label);m_variables->addItem(item);}refreshProperties(m_variables->currentRow());}
void MainWindow::refreshProperties(int r){m_properties->clear();if(r<0||r>=m_data.variables().size())return;const auto& v=m_data.variables()[r];m_properties->addTopLevelItem(new QTreeWidgetItem({"Name",v.name}));m_properties->addTopLevelItem(new QTreeWidgetItem({"Label",v.label}));m_properties->addTopLevelItem(new QTreeWidgetItem({"Type",variableTypeName(v.type)}));m_properties->addTopLevelItem(new QTreeWidgetItem({"Format",v.format}));m_properties->addTopLevelItem(new QTreeWidgetItem({"Missing values",v.missingValues.join(", ")}));m_properties->addTopLevelItem(new QTreeWidgetItem({"Observations",QString::number(m_data.rowCount())}));m_properties->addTopLevelItem(new QTreeWidgetItem({"Missing count",QString::number(m_data.missingCount(r))}));}

bool MainWindow::setCellFromText(int r,int c,const QString& text,QString* error){if(!m_data.validateValue(c,text,error))return false;if(!m_data.setValue(r,c,text)){if(error&&error->isEmpty())*error="Value could not be stored.";return false;}return true;}
void MainWindow::cellChanged(int viewRow,int c){if(m_refreshing)return;int r=dataRowForViewRow(viewRow);if(r<0)return;auto* item=m_grid->item(viewRow,c);if(!item)return;const QString newV=item->text(),oldV=m_data.value(r,c).toString();if(newV==oldV)return;QString err;if(!m_data.validateValue(c,newV,&err)){m_refreshing=true;item->setText(oldV);m_refreshing=false;QMessageBox::warning(this,"Invalid value",err);return;}m_data.setValue(r,c,newV);m_state.setDirty(true);m_undoStack->push(new CellEditCommand(this,r,c,oldV,newV));updateStatus();}

void MainWindow::copySelection(){const auto ranges=m_grid->selectedRanges();if(ranges.isEmpty())return;const auto rg=ranges.first();QString out;for(int r=rg.topRow();r<=rg.bottomRow();++r){QStringList cells;for(int c=rg.leftColumn();c<=rg.rightColumn();++c)cells<<(m_grid->item(r,c)?m_grid->item(r,c)->text():QString());out+=cells.join('\t');if(r<rg.bottomRow())out+='\n';}QApplication::clipboard()->setText(out);statusBar()->showMessage("Selection copied");}
void MainWindow::pasteSelection(){const auto text=QApplication::clipboard()->text();if(text.isEmpty())return;int startR=m_grid->currentRow(),startC=m_grid->currentColumn();if(startR<0||startC<0)return;const auto lines=text.split('\n');int changed=0;for(int i=0;i<lines.size();++i){const auto cells=lines[i].split('\t');for(int j=0;j<cells.size();++j){int vr=startR+i, c=startC+j;if(vr>=m_grid->rowCount()||c>=m_grid->columnCount())continue;int r=dataRowForViewRow(vr);QString err;if(!setCellFromText(r,c,cells[j],&err)){QMessageBox::warning(this,"Paste",QString("Paste stopped at row %1, column %2: %3").arg(vr+1).arg(c+1).arg(err));updateStatus();return;}++changed;}}if(changed){m_state.setDirty(true);refreshDataView();m_grid->setCurrentCell(startR,startC);}statusBar()->showMessage(QString("%1 cells pasted").arg(changed));}

void MainWindow::addRow(){QVector<QVariant> vals;vals.resize(m_data.columnCount());if(!m_data.appendRow(vals))return;m_state.setDirty(true);refreshDataView();m_grid->setCurrentCell(m_grid->rowCount()-1,0);}
void MainWindow::insertRow(){int vr=m_grid->currentRow();int r=vr<0?m_data.rowCount():dataRowForViewRow(vr);QVector<QVariant> vals;vals.resize(m_data.columnCount());if(!m_data.insertRow(r,vals))return;m_state.setDirty(true);refreshDataView();}
void MainWindow::deleteRows(){auto ranges=m_grid->selectedRanges();if(ranges.isEmpty())return;QSet<int> rows;for(const auto& rg:ranges)for(int vr=rg.topRow();vr<=rg.bottomRow();++vr){int r=dataRowForViewRow(vr);if(r>=0)rows.insert(r);}if(rows.isEmpty())return;if(QMessageBox::question(this,"Delete Rows",QString("Delete %1 selected row(s)?").arg(rows.size()))!=QMessageBox::Yes)return;QVector<int> sorted=rows.values().toVector();std::sort(sorted.rbegin(),sorted.rend());for(int r:sorted)m_data.removeRow(r);m_state.setDirty(true);refreshDataView();}
void MainWindow::sortAscending(){int c=m_grid->currentColumn();if(c<0)return;m_data.sortByColumn(c,true);m_state.setDirty(true);refreshDataView();}
void MainWindow::sortDescending(){int c=m_grid->currentColumn();if(c<0)return;m_data.sortByColumn(c,false);m_state.setDirty(true);refreshDataView();}
void MainWindow::showDatasetInfo(){QString text=QString("Dataset\n\nRows: %1\nVariables: %2\nMissing cells: %3\n\nVariables:\n").arg(m_data.rowCount()).arg(m_data.columnCount());int missing=0;for(int c=0;c<m_data.columnCount();++c){missing+=m_data.missingCount(c);text+=QString("• %1 — %2 — missing %3\n").arg(m_data.variables()[c].name,variableTypeName(m_data.variables()[c].type)).arg(m_data.missingCount(c));}m_output->setPlainText(text);m_tabs->setCurrentWidget(m_output);}
void MainWindow::undo(){m_undoStack->undo();m_state.setDirty(true);refreshDataView();}
void MainWindow::redo(){m_undoStack->redo();m_state.setDirty(true);refreshDataView();}

void MainWindow::applyFilter(){const QString q=m_filterEdit->text().trimmed();QVector<int> rows=m_data.filteredRows(q);refreshDataView(rows);statusBar()->showMessage(QString("%1 of %2 rows shown").arg(rows.size()).arg(m_data.rowCount()));}
void MainWindow::clearFilter(){m_filterEdit->clear();refreshDataView();}
void MainWindow::replaceText(){bool ok=false;const auto find=QInputDialog::getText(this,"Find / Replace","Find:",QLineEdit::Normal,"",&ok);if(!ok||find.isEmpty())return;const auto repl=QInputDialog::getText(this,"Find / Replace","Replace with:",QLineEdit::Normal,"",&ok);if(!ok)return;int count=0;for(int r=0;r<m_data.rowCount();++r)for(int c=0;c<m_data.columnCount();++c){auto s=m_data.value(r,c).toString();if(s.contains(find,Qt::CaseInsensitive)){s.replace(find,repl,Qt::CaseInsensitive);QString err;if(m_data.validateValue(c,s,&err)) {m_data.setValue(r,c,s);++count;}}}if(count){m_state.setDirty(true);refreshDataView();}m_output->setPlainText(QString("Find / Replace complete. %1 cells changed.").arg(count));}
void MainWindow::updateTitle(){setWindowTitle(QString("StatPro Analytics — %1%2").arg(m_state.projectName(),m_state.dirty()?" *":""));if(m_projectTitle)m_projectTitle->setText(m_state.projectName()+(m_state.dirty()?" *":""));}
void MainWindow::updateStatus(){statusBar()->showMessage(QString("%1 observations • %2 variables • Ready • Offline mode").arg(m_data.rowCount()).arg(m_data.columnCount()));}
bool MainWindow::confirmSaveIfDirty(){if(!m_state.dirty())return true;const auto ans=QMessageBox::warning(this,"Unsaved Changes","This project has unsaved changes.",QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel);if(ans==QMessageBox::Save){saveProject();return !m_state.dirty();}return ans==QMessageBox::Discard;}
void MainWindow::closeEvent(QCloseEvent* e){if(confirmSaveIfDirty())e->accept();else e->ignore();}
void MainWindow::applyTheme(){
 if(m_state.darkMode())setStyleSheet(R"(QMainWindow,QDockWidget,QTabWidget::pane{background:#20242a;color:#e8eaed}QToolBar{background:#292e36;border:1px solid #3b414b;padding:5px;spacing:4px}QToolButton{color:#e8eaed;padding:7px 9px;border-radius:5px}QToolButton:hover{background:#3a414c}QTableWidget,QListWidget,QTreeWidget,QPlainTextEdit,QLineEdit,QComboBox{background:#252a31;color:#e8eaed;border:1px solid #414752}QHeaderView::section{background:#303640;color:#e8eaed;padding:5px}QLabel#ProjectTitle{font-size:18px;font-weight:600;padding:3px}QPushButton{color:#e8eaed;background:#303640;border:1px solid #4b5360;padding:6px 12px;border-radius:4px}QPushButton:hover{background:#3a414c})");
 else setStyleSheet(R"(QMainWindow,QDockWidget,QTabWidget::pane{background:#f4f6f8;color:#20242a}QToolBar{background:#ffffff;border:1px solid #d8dde3;padding:5px;spacing:4px}QToolButton{color:#20242a;padding:7px 9px;border-radius:5px}QToolButton:hover{background:#e9eef5}QTableWidget,QListWidget,QTreeWidget,QPlainTextEdit,QLineEdit,QComboBox{background:#ffffff;color:#20242a;border:1px solid #d8dde3}QHeaderView::section{background:#eef1f4;color:#20242a;padding:5px}QLabel#ProjectTitle{font-size:18px;font-weight:600;padding:3px}QPushButton{color:#20242a;background:#ffffff;border:1px solid #b8c0c9;padding:6px 12px;border-radius:4px}QPushButton:hover{background:#e9eef5})");
}
}
