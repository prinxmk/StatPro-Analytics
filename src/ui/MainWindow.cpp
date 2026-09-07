#include "MainWindow.h"
#include "../data/CsvImporter.h"
#include "../data/ProjectManager.h"
#include "../analysis/AnalysisEngine.h"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDockWidget>
#include <QSet>
#include <QComboBox>
#include <QDate>
#include <QColorDialog>
#include <QSettings>
#include <QAbstractItemView>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFont>
#include <QGroupBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QFrame>
#include <QMenu>
#include <QMenuBar>
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
#include <QGridLayout>
#include <QUndoCommand>
#include <algorithm>
#include <functional>
#include <cmath>

namespace StatPro {

class CellEditCommand : public QUndoCommand {
public:
    CellEditCommand(MainWindow* w,int r,int c,const QString& oldV,const QString& newV):m_w(w),m_r(r),m_c(c),m_old(oldV),m_new(newV){setText("Edit cell");}
    void undo() override {m_w->setCellFromText(m_r,m_c,m_old);}
    void redo() override {m_w->setCellFromText(m_r,m_c,m_new);}
private: MainWindow* m_w; int m_r,m_c; QString m_old,m_new;
};

MainWindow::MainWindow(QWidget* parent):QMainWindow(parent),m_undoStack(new QUndoStack(this)){
    resize(1550,920); setWindowTitle(QString("StatPro Analytics %1").arg(QApplication::applicationVersion())); buildInterface(); applyTheme();
    connect(&m_state,&AppState::dirtyChanged,this,&MainWindow::updateTitle);
    connect(&m_state,&AppState::dirtyChanged,this,&MainWindow::updateStatus);
}


void MainWindow::buildInterface(){
    buildMenus();

    auto* central=new QWidget;
    auto* layout=new QVBoxLayout(central);
    layout->setContentsMargins(8,6,8,6);
    layout->setSpacing(6);

    auto* filterRow=new QHBoxLayout;
    m_filterEdit=new QLineEdit;
    m_filterEdit->setPlaceholderText("Filter rows… (text search or: age > 30, sex == Male)");
    auto* apply=new QPushButton("Apply");
    auto* clear=new QPushButton("Clear");
    auto* replace=new QPushButton("Find / Replace");
    filterRow->addWidget(new QLabel("Data filter:"));
    filterRow->addWidget(m_filterEdit,1);
    filterRow->addWidget(apply);
    filterRow->addWidget(clear);
    filterRow->addWidget(replace);
    layout->addLayout(filterRow);

    connect(apply,&QPushButton::clicked,this,&MainWindow::applyFilter);
    connect(clear,&QPushButton::clicked,this,&MainWindow::clearFilter);
    connect(replace,&QPushButton::clicked,this,&MainWindow::replaceText);

    m_tabs=new QTabWidget;
    m_grid=new QTableWidget;
    m_grid->setAlternatingRowColors(false);
    m_grid->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_grid->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_grid->setEditTriggers(QAbstractItemView::DoubleClicked|QAbstractItemView::EditKeyPressed|QAbstractItemView::AnyKeyPressed);
    m_grid->setSortingEnabled(false);
    m_grid->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_grid->verticalHeader()->setDefaultSectionSize(24);
    m_grid->horizontalHeader()->setSectionsMovable(false);
    m_grid->horizontalHeader()->setSortIndicatorShown(false);
    m_grid->horizontalHeader()->setSectionsClickable(true);
    m_grid->setCornerButtonEnabled(true);
    m_grid->setWordWrap(false);
    m_grid->setTabKeyNavigation(true);

    connect(m_grid->horizontalHeader(), &QHeaderView::sectionClicked, this, [this](int c){
        if (c < 0 || c >= m_grid->columnCount()) return;
        m_grid->clearSelection();
        m_grid->selectColumn(c);
        if (m_grid->rowCount() > 0) m_grid->setCurrentCell(0, c);
        selectVariableColumn(c);
        updateStatus();
    });
    connect(m_grid, &QTableWidget::currentCellChanged, this, [this](int, int c, int, int){
        if(c>=0) selectVariableColumn(c);
        updateStatus();
    });
    connect(m_grid, &QTableWidget::itemSelectionChanged, this, &MainWindow::updateStatus);

    m_tabs->addTab(m_grid,"Data Editor");
    connect(m_grid,&QTableWidget::cellChanged,this,&MainWindow::cellChanged);

    auto* outputPage=new QWidget;
    auto* outputLayout=new QVBoxLayout(outputPage);
    outputLayout->setContentsMargins(10,10,10,10);
    outputLayout->setSpacing(6);
    m_outputTitle=new QLabel("Results / Output");
    m_outputTitle->setObjectName("ResultsTitle");
    m_outputTitle->setStyleSheet("font-size:16px;font-weight:600;");
    m_outputSummary=new QLabel;
    m_outputSummary->setObjectName("ResultsSummary");
    m_outputSummary->setWordWrap(true);
    m_output=new QTableWidget;
    m_output->setAlternatingRowColors(false);
    m_output->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_output->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_output->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_output->setWordWrap(false);
    m_output->setCornerButtonEnabled(true);
    m_output->verticalHeader()->setDefaultSectionSize(26);
    m_output->horizontalHeader()->setSectionsMovable(false);
    m_output->horizontalHeader()->setStretchLastSection(false);
    m_outputNote=new QLabel;
    m_outputNote->setObjectName("ResultsNote");
    m_outputNote->setWordWrap(true);
    outputLayout->addWidget(m_outputTitle);
    outputLayout->addWidget(m_outputSummary);
    outputLayout->addWidget(m_output,1);
    outputLayout->addWidget(m_outputNote);
    m_tabs->addTab(outputPage,"Results / Output");
    layout->addWidget(m_tabs,1);
    setCentralWidget(central);

    buildVariablesPanel();
    buildPropertiesPanel();
    buildDataCommandPanel();

    if(m_propertiesDock && m_commandsDock){
        splitDockWidget(m_propertiesDock,m_commandsDock,Qt::Vertical);
        resizeDocks({m_propertiesDock,m_commandsDock},{420,360},Qt::Vertical);
    }

    buildStatusBar();


    updateTitle();
    updateStatus();
}



void MainWindow::buildMenus(){
    auto addAction=[this](QMenu* menu,const QString& text,auto fn,const QKeySequence& shortcut=QKeySequence()){
        QAction* action=menu->addAction(text);
        if(!shortcut.isEmpty())action->setShortcut(shortcut);
        connect(action,&QAction::triggered,this,fn);
        return action;
    };

    auto* fileMenu=menuBar()->addMenu("&File");
    addAction(fileMenu,"New Project",[this]{newProject();},QKeySequence::New);
    addAction(fileMenu,"Open Project…",[this]{openProject();},QKeySequence::Open);
    addAction(fileMenu,"Save Project",[this]{saveProject();},QKeySequence::Save);
    addAction(fileMenu,"Save Project As…",[this]{saveProjectAs();},QKeySequence::SaveAs);
    fileMenu->addSeparator();
    addAction(fileMenu,"Import Data…",[this]{importCsv();});
    addAction(fileMenu,"Export Data…",[this]{exportCsv();});
    fileMenu->addSeparator();
    addAction(fileMenu,"Exit",[this]{close();},QKeySequence::Quit);

    auto* dataMenu=menuBar()->addMenu("&Data");

    auto* variableMenu=dataMenu->addMenu("Variables");
    addAction(variableMenu,"Add Variable…",[this]{addVariable();});
    addAction(variableMenu,"Edit Variable…",[this]{editVariable();});
    addAction(variableMenu,"Delete Variable",[this]{deleteVariable();});

    auto* rowMenu=dataMenu->addMenu("Rows");
    addAction(rowMenu,"Add Row",[this]{addRow();},QKeySequence(Qt::CTRL | Qt::Key_Insert));
    addAction(rowMenu,"Insert Row",[this]{insertRow();});
    addAction(rowMenu,"Delete Selected Row(s)",[this]{deleteRows();});

    auto* clipboardMenu=dataMenu->addMenu("Clipboard");
    addAction(clipboardMenu,"Copy",[this]{copySelection();},QKeySequence::Copy);
    addAction(clipboardMenu,"Paste",[this]{pasteSelection();},QKeySequence::Paste);
    clipboardMenu->addSeparator();
    addAction(clipboardMenu,"Undo",[this]{undo();},QKeySequence::Undo);
    addAction(clipboardMenu,"Redo",[this]{redo();},QKeySequence::Redo);

    auto* sortMenu=dataMenu->addMenu("Sort");
    addAction(sortMenu,"Ascending",[this]{sortAscending();});
    addAction(sortMenu,"Descending",[this]{sortDescending();});

    dataMenu->addSeparator();
    addAction(dataMenu,"Find / Replace…",[this]{replaceText();});
    addAction(dataMenu,"Dataset Information",[this]{showDatasetInfo();});

    auto* analysisMenu=menuBar()->addMenu("&Analysis");
    auto* describeMenu=analysisMenu->addMenu("Describe");
    addAction(describeMenu,"Descriptive Statistics…",[this]{runDescriptiveStatistics();});
    addAction(describeMenu,"Frequencies…",[this]{runFrequencies();});
    addAction(describeMenu,"Summary by Group…",[this]{runSummaryByGroup();});
    auto* testsMenu=analysisMenu->addMenu("Tests");
    addAction(testsMenu,"Pearson Correlation…",[this]{runPearsonCorrelation();});
    addAction(testsMenu,"One-Sample t Test…",[this]{runOneSampleTTest();});
    addAction(testsMenu,"Independent-Samples t Test…",[this]{runIndependentTTest();});
    addAction(testsMenu,"Paired-Samples t Test…",[this]{runPairedTTest();});
    addAction(testsMenu,"Chi-Square Test of Independence…",[this]{runChiSquare();});
    addAction(testsMenu,"One-Way ANOVA…",[this]{runOneWayAnova();});
    auto* nonparametricMenu=testsMenu->addMenu("Nonparametric Tests");
    addAction(nonparametricMenu,"Mann–Whitney U Test…",[this]{runMannWhitneyU();});
    addAction(nonparametricMenu,"Wilcoxon Signed-Rank Test…",[this]{runWilcoxonSignedRank();});
    addAction(nonparametricMenu,"Kruskal–Wallis Test…",[this]{runKruskalWallis();});
    addAction(nonparametricMenu,"Spearman Rank Correlation…",[this]{runSpearmanCorrelation();});
    auto* regressionMenu=analysisMenu->addMenu("Regression");
    addAction(regressionMenu,"Simple Linear Regression…",[this]{runSimpleLinearRegression();});
    addAction(regressionMenu,"Multiple Linear Regression…",[this]{runMultipleLinearRegression();});
    addAction(regressionMenu,"Multiple Linear Regression with Categorical Predictors…",[this]{runRegressionWithCategoricalPredictors();});
    addAction(regressionMenu,"Regression Prediction & Fitted Values…",[this]{runRegressionPrediction();});
    addAction(regressionMenu,"Logistic Regression…",[this]{runLogisticRegression();});
    auto* glmMenu=regressionMenu->addMenu("Generalized Linear Models");
    addAction(glmMenu,"Poisson Regression…",[this]{runPoissonRegression();});
    addAction(glmMenu,"Negative Binomial Regression…",[this]{runNegativeBinomialRegression();});
    for(const auto& group : QStringList{
            "Data","Cleaning","Transform",
            "Time Series","Econometrics","Survival","Survey","Multivariate",
            "Machine Learning","Graphs","Diagnostics","Interpret","Reports"}) {
        auto* groupMenu=analysisMenu->addMenu(group);
        if(group=="Diagnostics") {
            addAction(groupMenu,"Regression Diagnostics…",[this]{runRegressionDiagnostics();});
        } else if(group=="Time Series") {
            addAction(groupMenu,"Time Series Analysis…",[this]{runTimeSeriesAnalysis();});
        } else if(group=="Econometrics") {
            addAction(groupMenu,"Robust OLS (HC1) & Breusch–Pagan…",[this]{runEconometricRobustOls();});
            addAction(groupMenu,"Augmented Dickey–Fuller Unit Root Test…",[this]{runADFTest();});
        } else {
            QAction* placeholder=groupMenu->addAction(QString("Open %1 module").arg(group));
            connect(placeholder,&QAction::triggered,this,[this,group]{
                showResultMessage(group, "The " + group + " module is available in the analysis menu. Statistical procedures will be added in the analysis-engine phases.");
                m_tabs->setCurrentWidget(m_output);
                statusBar()->showMessage(group + " module selected",3000);
            });
        }
    }

    auto* viewMenu=menuBar()->addMenu("&View");
    addAction(viewMenu,"Light / Dark Theme",[this]{toggleTheme();});
    addAction(viewMenu,"Results Table Formatting…",[this]{formatResultsTables();});
    viewMenu->addSeparator();

    // Dock toggle actions are added after the docks exist in buildInterface().
    connect(viewMenu,&QMenu::aboutToShow,this,[this,viewMenu]{
        if(viewMenu->property("dockTogglesAdded").toBool())return;
        viewMenu->addSeparator();
        if(m_variablesDock)viewMenu->addAction(m_variablesDock->toggleViewAction());
        if(m_propertiesDock)viewMenu->addAction(m_propertiesDock->toggleViewAction());
        if(m_commandsDock)viewMenu->addAction(m_commandsDock->toggleViewAction());
        viewMenu->setProperty("dockTogglesAdded",true);
    });
}



void MainWindow::buildVariablesPanel(){
    m_variablesDock=new QDockWidget("Variables / Elements",this);
    m_variablesDock->setObjectName("VariablesDock");
    auto* panel=new QWidget;
    auto* layout=new QVBoxLayout(panel);
    layout->setContentsMargins(5,5,5,5);
    layout->setSpacing(5);

    m_variables=new QListWidget;
    m_variables->setAlternatingRowColors(false);
    m_variables->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_variables,1);

    auto* buttons=new QHBoxLayout;
    auto* edit=new QPushButton("Edit");
    auto* remove=new QPushButton("Delete");
    buttons->addWidget(edit);
    buttons->addWidget(remove);
    layout->addLayout(buttons);

    m_variablesDock->setWidget(panel);
    addDockWidget(Qt::LeftDockWidgetArea,m_variablesDock);

    connect(m_variables,&QListWidget::currentRowChanged,this,[this](int r){
        refreshProperties(r);
        updateStatus();
    });
    connect(m_variables,&QListWidget::itemDoubleClicked,this,[this](QListWidgetItem*){editVariable();});
    connect(edit,&QPushButton::clicked,this,&MainWindow::editVariable);
    connect(remove,&QPushButton::clicked,this,&MainWindow::deleteVariable);
}

void MainWindow::buildPropertiesPanel(){
    m_propertiesDock=new QDockWidget("Properties",this);
    m_propertiesDock->setObjectName("PropertiesDock");
    m_properties=new QTreeWidget;
    m_properties->setHeaderLabels({"Property","Value"});
    m_properties->setAlternatingRowColors(false);
    m_propertiesDock->setWidget(m_properties);
    addDockWidget(Qt::RightDockWidgetArea,m_propertiesDock);
}

void MainWindow::buildDataCommandPanel(){
    m_commandsDock=new QDockWidget("Data Editor Commands",this);
    m_commandsDock->setObjectName("DataCommandsDock");
    m_commandsDock->setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);

    auto* panel=new QWidget;
    auto* layout=new QVBoxLayout(panel);
    layout->setContentsMargins(7,7,7,7);
    layout->setSpacing(8);

    auto addSection=[this,layout](const QString& title,const QList<QPair<QString,std::function<void()>>>& actions){
        auto* box=new QGroupBox(title);
        auto* boxLayout=new QVBoxLayout(box);
        boxLayout->setContentsMargins(7,7,7,7);
        boxLayout->setSpacing(5);
        for(const auto& item:actions){
            auto* button=new QPushButton(item.first);
            button->setMinimumHeight(28);
            connect(button,&QPushButton::clicked,this,item.second);
            boxLayout->addWidget(button);
        }
        layout->addWidget(box);
    };

    addSection("Variables",{
        {"Add Variable",[this]{addVariable();}},
        {"Edit Variable",[this]{editVariable();}},
        {"Delete Variable",[this]{deleteVariable();}}
    });

    addSection("Rows",{
        {"Add Row",[this]{addRow();}},
        {"Insert Row",[this]{insertRow();}},
        {"Delete Row(s)",[this]{deleteRows();}}
    });

    addSection("Clipboard",{
        {"Copy",[this]{copySelection();}},
        {"Paste",[this]{pasteSelection();}},
        {"Undo",[this]{undo();}},
        {"Redo",[this]{redo();}}
    });

    addSection("Order & Dataset",{
        {"Sort Ascending",[this]{sortAscending();}},
        {"Sort Descending",[this]{sortDescending();}},
        {"Dataset Info",[this]{showDatasetInfo();}}
    });

    layout->addStretch(1);
    m_commandsDock->setWidget(panel);
    addDockWidget(Qt::RightDockWidgetArea,m_commandsDock);
}

void MainWindow::buildStatusBar(){
    auto* bar=statusBar();
    bar->setSizeGripEnabled(true);

    m_statusState=new QLabel("Ready");
    m_statusRows=new QLabel("0 observations");
    m_statusVariables=new QLabel("0 variables");
    m_statusSelection=new QLabel("No selection");
    m_statusMode=new QLabel("Offline");

    for(auto* label : {m_statusRows,m_statusVariables,m_statusSelection,m_statusMode}){
        label->setContentsMargins(8,0,8,0);
        bar->addPermanentWidget(label);
    }
    bar->addWidget(m_statusState,1);
}


void MainWindow::newProject(){if(!confirmSaveIfDirty())return;m_data.clear();m_state.setProjectPath("");m_state.setProjectName("Untitled Project");m_state.setDirty(false);m_undoStack->clear();m_filterEdit->clear();refreshDataView();refreshVariables();showResultMessage("Project", "New project created.");}
void MainWindow::openProject(){if(!confirmSaveIfDirty())return;const auto path=QFileDialog::getOpenFileName(this,"Open StatPro Project",{},"StatPro Project (*.stpro)");if(path.isEmpty())return;QString name,error;if(!ProjectManager::openProject(path,name,m_data,&error)){QMessageBox::critical(this,"Open Project",error);return;}m_state.setProjectPath(path);m_state.setProjectName(name);m_state.setDirty(false);m_state.addRecentProject(path);m_undoStack->clear();m_filterEdit->clear();refreshDataView();refreshVariables();showResultMessage("Project", "Project opened: "+path);}
void MainWindow::saveProject(){if(m_state.projectPath().isEmpty()){saveProjectAs();return;}QString error;if(!ProjectManager::saveProject(m_state.projectPath(),m_state.projectName(),m_data,&error))QMessageBox::critical(this,"Save Project",error);else{m_state.setDirty(false);m_state.addRecentProject(m_state.projectPath());statusBar()->showMessage("Project saved");}}
void MainWindow::saveProjectAs(){auto path=QFileDialog::getSaveFileName(this,"Save StatPro Project",{},"StatPro Project (*.stpro)");if(path.isEmpty())return;if(!path.endsWith(".stpro",Qt::CaseInsensitive))path += ".stpro";m_state.setProjectPath(path);m_state.setProjectName(QFileInfo(path).completeBaseName());updateTitle();saveProject();}
void MainWindow::importCsv(){const auto path=QFileDialog::getOpenFileName(this,"Import Data",{},"Supported data (*.csv *.tsv *.txt);;CSV files (*.csv);;Tab-separated files (*.tsv);;Text files (*.txt)");if(path.isEmpty())return;QString error;if(!CsvImporter::importFile(path,m_data,&error)){QMessageBox::critical(this,"Import Data",error);return;}m_state.setProjectPath("");m_state.setProjectName(QFileInfo(path).completeBaseName());m_state.setDirty(true);m_undoStack->clear();m_filterEdit->clear();refreshDataView();refreshVariables();showResultMessage("Import", "Data imported successfully.\n"+path);m_tabs->setCurrentWidget(m_grid);}
void MainWindow::exportCsv(){QString selectedFilter;auto path=QFileDialog::getSaveFileName(this,"Export Data",{},"CSV files (*.csv);;Tab-separated files (*.tsv);;Text files (*.txt)",&selectedFilter);if(path.isEmpty())return;QChar delimiter=',';if(selectedFilter.startsWith("Tab-separated")||QFileInfo(path).suffix().compare("tsv",Qt::CaseInsensitive)==0)delimiter='\t';QString error;if(!m_data.saveDelimited(path,delimiter,&error))QMessageBox::critical(this,"Export Data",error);else statusBar()->showMessage("Data exported");}
void MainWindow::toggleTheme(){m_state.setDarkMode(!m_state.darkMode());applyTheme();}

void MainWindow::addVariable(){
    QDialog d(this); d.setWindowTitle("Add Variable"); auto* form=new QFormLayout(&d); auto* name=new QLineEdit; auto* label=new QLineEdit; auto* type=new QComboBox; type->addItems({"String","Numeric","Date","Boolean"}); auto* missing=new QLineEdit; missing->setPlaceholderText("e.g. 99, -99");
    form->addRow("Name *",name);form->addRow("Label",label);form->addRow("Type",type);form->addRow("Missing values",missing);auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form->addRow(buttons);connect(buttons,&QDialogButtonBox::accepted,&d,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);if(d.exec()!=QDialog::Accepted)return;
    Variable v;v.name=name->text().trimmed();v.label=label->text().trimmed();v.type=variableTypeFromName(type->currentText());v.missingValues=missing->text().split(',',Qt::SkipEmptyParts);for(auto& x:v.missingValues)x=x.trimmed();if(v.name.isEmpty()||!m_data.addVariable(v)){QMessageBox::warning(this,"Add Variable","The variable name is empty or already exists.");return;}m_state.setDirty(true);refreshDataView();refreshVariables();m_variables->setCurrentRow(m_data.columnCount()-1);
}

void MainWindow::editVariable(){
    const int c=selectedVariableColumn();
    if(c<0 || c>=m_data.columnCount()){
        QMessageBox::information(this,"Variable Editor","Select a variable in the Variables panel or select a Data Editor column first.");
        return;
    }

    const Variable original=m_data.variables()[c];
    QDialog d(this);
    d.setWindowTitle("Variable Editor");
    d.setMinimumWidth(430);
    auto* form=new QFormLayout(&d);

    auto* name=new QLineEdit(original.name);
    auto* label=new QLineEdit(original.label);
    auto* type=new QComboBox;
    type->addItems({"String","Numeric","Date","Boolean"});
    type->setCurrentText(variableTypeName(original.type));
    auto* format=new QLineEdit(original.format);
    auto* missing=new QLineEdit(original.missingValues.join(", "));
    missing->setPlaceholderText("e.g. 99, -99, NA");

    form->addRow("Name *",name);
    form->addRow("Label",label);
    form->addRow("Type",type);
    form->addRow("Format",format);
    form->addRow("Missing values",missing);

    auto* hint=new QLabel("Changing a variable to Numeric, Date or Boolean validates all existing values.");
    hint->setWordWrap(true);
    form->addRow(hint);

    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    form->addRow(buttons);
    connect(buttons,&QDialogButtonBox::accepted,&d,&QDialog::accept);
    connect(buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);

    if(d.exec()!=QDialog::Accepted)return;

    Variable v=original;
    v.name=name->text().trimmed();
    v.label=label->text().trimmed();
    v.type=variableTypeFromName(type->currentText());
    v.format=format->text().trimmed();
    v.missingValues=missing->text().split(',',Qt::SkipEmptyParts);
    for(auto& x:v.missingValues)x=x.trimmed();

    if(v.name.isEmpty()){
        QMessageBox::warning(this,"Variable Editor","Variable name cannot be empty.");
        return;
    }
    QString variableError;
    if(!m_data.setVariable(c,v,&variableError)){
        QMessageBox::warning(this,"Variable Editor",variableError.isEmpty()?"The variable could not be updated.":variableError);
        return;
    }

    m_state.setDirty(true);
    refreshDataView();
    refreshVariables();
    m_variables->setCurrentRow(c);
    statusBar()->showMessage(QString("Variable '%1' updated").arg(v.name));
}

void MainWindow::deleteVariable(){
    const int c=selectedVariableColumn();
    if(c<0 || c>=m_data.columnCount()){QMessageBox::information(this,"Delete Variable","Select a variable in the Variables panel or a column in the Data Editor first.");return;}
    const QString name=m_data.variables()[c].name;
    if(QMessageBox::question(this,"Delete Variable",QString("Delete variable '%1' and all of its data?").arg(name))==QMessageBox::Yes){
        if(!m_data.removeVariable(c)){QMessageBox::warning(this,"Delete Variable","The variable could not be deleted.");return;}
        m_state.setDirty(true);refreshDataView();refreshVariables();
        if(m_data.columnCount()>0)selectVariableColumn(qMin(c,m_data.columnCount()-1));
        statusBar()->showMessage(QString("Variable '%1' deleted").arg(name));
    }
}


int MainWindow::selectedVariableColumn() const{
    if(m_variables){const int r=m_variables->currentRow();if(r>=0 && r<m_data.columnCount())return r;}
    if(m_grid){const int c=m_grid->currentColumn();if(c>=0 && c<m_data.columnCount())return c;}
    return -1;
}
void MainWindow::selectVariableColumn(int c){
    if(c<0 || c>=m_data.columnCount())return;
    if(m_variables && m_variables->currentRow()!=c)m_variables->setCurrentRow(c);
    refreshProperties(c);
}

QVector<int> MainWindow::visibleRows() const { QVector<int> rows; if (m_filterEdit->text().trimmed().isEmpty()) { for (int i=0;i<m_data.rowCount();++i) rows << i; } else rows = m_data.filteredRows(m_filterEdit->text()); return rows; }
int MainWindow::dataRowForViewRow(int viewRow) const{return viewRow>=0&&viewRow<m_visibleRows.size()?m_visibleRows[viewRow]:-1;}
QString MainWindow::displayValue(int r,int c) const{return m_data.value(r,c).toString();}

void MainWindow::refreshDataView(){refreshDataView(visibleRows());}
void MainWindow::refreshDataView(const QVector<int>& rows){
    m_refreshing=true;m_visibleRows=rows;m_grid->setUpdatesEnabled(false);m_grid->clear();m_grid->setRowCount(rows.size());m_grid->setColumnCount(m_data.columnCount());m_grid->setHorizontalHeaderLabels(m_data.columnNames());
    for(int c=0;c<m_data.columnCount();++c){auto* h=m_grid->horizontalHeaderItem(c);if(h){const auto& v=m_data.variables()[c];QString tip=QString("Name: %1\nType: %2").arg(v.name,variableTypeName(v.type));if(!v.label.isEmpty())tip+=QString("\nLabel: %1").arg(v.label);if(!v.missingValues.isEmpty())tip+=QString("\nMissing: %1").arg(v.missingValues.join(", "));h->setToolTip(tip);}}
    for(int rr=0;rr<rows.size();++rr){m_grid->setVerticalHeaderItem(rr,new QTableWidgetItem(QString::number(rows[rr]+1)));for(int c=0;c<m_data.columnCount();++c){auto* item=new QTableWidgetItem(displayValue(rows[rr],c));if(m_data.isMissing(rows[rr],c)){QFont f=item->font();f.setItalic(true);item->setFont(f);item->setToolTip("Missing value (declared or blank)");}m_grid->setItem(rr,c,item);}}
    m_grid->setUpdatesEnabled(true);m_refreshing=false;m_grid->resizeColumnsToContents();updateStatus();
}
void MainWindow::refreshVariables(){m_variables->clear();for(const auto& v:m_data.variables()){QString text=v.name;if(!v.label.isEmpty())text+=QString("  —  %1").arg(v.label);auto* item=new QListWidgetItem(text);item->setData(Qt::UserRole,v.name);if(!v.label.isEmpty())item->setToolTip(v.label);m_variables->addItem(item);}refreshProperties(m_variables->currentRow());}
void MainWindow::refreshProperties(int r){m_properties->clear();if(r<0||r>=m_data.variables().size())return;const auto& v=m_data.variables()[r];m_properties->addTopLevelItem(new QTreeWidgetItem({"Name",v.name}));m_properties->addTopLevelItem(new QTreeWidgetItem({"Label",v.label}));m_properties->addTopLevelItem(new QTreeWidgetItem({"Type",variableTypeName(v.type)}));m_properties->addTopLevelItem(new QTreeWidgetItem({"Format",v.format}));m_properties->addTopLevelItem(new QTreeWidgetItem({"Missing values",v.missingValues.join(", ")}));m_properties->addTopLevelItem(new QTreeWidgetItem({"Observations",QString::number(m_data.rowCount())}));m_properties->addTopLevelItem(new QTreeWidgetItem({"Missing count",QString::number(m_data.missingCount(r))}));}

bool MainWindow::setCellFromText(int r,int c,const QString& text,QString* error){if(!m_data.validateValue(c,text,error))return false;if(!m_data.setValue(r,c,text)){if(error&&error->isEmpty())*error="Value could not be stored.";return false;}return true;}
void MainWindow::cellChanged(int viewRow,int c){if(m_refreshing)return;int r=dataRowForViewRow(viewRow);if(r<0)return;auto* item=m_grid->item(viewRow,c);if(!item)return;const QString newV=item->text(),oldV=m_data.value(r,c).toString();if(newV==oldV)return;QString err;if(!m_data.validateValue(c,newV,&err)){m_refreshing=true;item->setText(oldV);m_refreshing=false;QMessageBox::warning(this,"Invalid value",err);return;}m_data.setValue(r,c,newV);m_state.setDirty(true);m_undoStack->push(new CellEditCommand(this,r,c,oldV,newV));updateStatus();}

void MainWindow::copySelection(){
    const auto ranges=m_grid->selectedRanges();
    if(ranges.isEmpty())return;
    const auto rg=ranges.first();
    QStringList lines;
    for(int r=rg.topRow();r<=rg.bottomRow();++r){
        QStringList cells;
        for(int c=rg.leftColumn();c<=rg.rightColumn();++c)
            cells << (m_grid->item(r,c)?m_grid->item(r,c)->text():QString());
        lines << cells.join('\t');
    }
    QApplication::clipboard()->setText(lines.join('\n'));
    statusBar()->showMessage(QString("Copied %1 × %2 cells").arg(rg.rowCount()).arg(rg.columnCount()));
}

void MainWindow::pasteSelection(){
    const QString text=QApplication::clipboard()->text();
    if(text.isEmpty())return;
    int startR=m_grid->currentRow(),startC=m_grid->currentColumn();
    if(startR<0)startR=0;
    if(startC<0)startC=0;
    if(m_data.columnCount()==0){
        QMessageBox::information(this,"Paste","Add at least one variable before pasting data.");
        return;
    }

    QString normalized=text;
    normalized.replace("\r\n","\n");
    normalized.replace('\r','\n');
    const auto lines=normalized.split('\n');
    int requiredRows=startR;
    for(const auto& line:lines)if(!line.isEmpty())++requiredRows;
    while(m_data.rowCount()<requiredRows){
        if(!m_data.appendRow()){
            QMessageBox::warning(this,"Paste","Could not add rows required for the pasted data.");
            return;
        }
    }

    int changed=0;
    QString firstError;
    for(int i=0;i<lines.size();++i){
        if(i>=lines.size())break;
        const auto cells=lines[i].split('\t');
        if(cells.size()==1 && cells[0].isEmpty() && i==lines.size()-1)continue;
        const int vr=startR+i;
        const bool filtered=!m_filterEdit->text().trimmed().isEmpty();
        const int dataRow=filtered ? dataRowForViewRow(vr) : vr;
        if(dataRow<0 || dataRow>=m_data.rowCount())continue;
        for(int j=0;j<cells.size();++j){
            const int c=startC+j;
            if(c<0 || c>=m_data.columnCount())continue;
            QString err;
            const QString oldValue=m_data.value(dataRow,c).toString();
            if(!setCellFromText(dataRow,c,cells[j],&err)){
                if(firstError.isEmpty())firstError=QString("Row %1, column %2: %3").arg(dataRow+1).arg(c+1).arg(err);
                continue;
            }
            if(oldValue!=cells[j])++changed;
        }
    }

    if(changed){
        m_state.setDirty(true);
        refreshDataView();
        if(startR<m_grid->rowCount() && startC<m_grid->columnCount())m_grid->setCurrentCell(startR,startC);
    }
    if(!firstError.isEmpty())
        QMessageBox::warning(this,"Paste",QString("Some cells were not pasted.\n\n%1").arg(firstError));
    statusBar()->showMessage(QString("%1 cells pasted").arg(changed));
}

void MainWindow::addRow(){
    QVector<QVariant> vals(m_data.columnCount());
    if(!m_data.appendRow(vals)){QMessageBox::warning(this,"Add Row","The row could not be added.");return;}
    m_state.setDirty(true);
    refreshDataView();
    if(m_grid->rowCount()>0 && m_grid->columnCount()>0)m_grid->setCurrentCell(m_grid->rowCount()-1,0);
    statusBar()->showMessage("New row added");
}
void MainWindow::insertRow(){
    int vr=m_grid->currentRow();
    int r=vr<0?m_data.rowCount():dataRowForViewRow(vr);
    if(r<0)r=m_data.rowCount();
    QVector<QVariant> vals(m_data.columnCount());
    if(!m_data.insertRow(r,vals)){QMessageBox::warning(this,"Insert Row","The row could not be inserted.");return;}
    m_state.setDirty(true);
    refreshDataView();
    if(m_grid->columnCount()>0 && r<m_grid->rowCount())m_grid->setCurrentCell(r,0);
    statusBar()->showMessage(QString("Row %1 inserted").arg(r+1));
}
void MainWindow::deleteRows(){auto ranges=m_grid->selectedRanges();if(ranges.isEmpty())return;QSet<int> rows;for(const auto& rg:ranges)for(int vr=rg.topRow();vr<=rg.bottomRow();++vr){int r=dataRowForViewRow(vr);if(r>=0)rows.insert(r);}if(rows.isEmpty())return;if(QMessageBox::question(this,"Delete Rows",QString("Delete %1 selected row(s)?").arg(rows.size()))!=QMessageBox::Yes)return;QVector<int> sorted=rows.values().toVector();std::sort(sorted.rbegin(),sorted.rend());for(int r:sorted)m_data.removeRow(r);m_state.setDirty(true);refreshDataView();}
void MainWindow::sortAscending(){
    int c=m_grid->currentColumn();
    if(c<0 && m_grid->columnCount()>0)c=0;
    if(c<0){QMessageBox::information(this,"Sort","Import data or add a variable first.");return;}
    if(!m_data.sortByColumn(c,true)){QMessageBox::warning(this,"Sort","The selected column could not be sorted.");return;}
    m_state.setDirty(true);refreshDataView();
    statusBar()->showMessage(QString("Sorted '%1' ascending").arg(m_data.variables()[c].name));
}
void MainWindow::sortDescending(){
    int c=m_grid->currentColumn();
    if(c<0 && m_grid->columnCount()>0)c=0;
    if(c<0){QMessageBox::information(this,"Sort","Import data or add a variable first.");return;}
    if(!m_data.sortByColumn(c,false)){QMessageBox::warning(this,"Sort","The selected column could not be sorted.");return;}
    m_state.setDirty(true);refreshDataView();
    statusBar()->showMessage(QString("Sorted '%1' descending").arg(m_data.variables()[c].name));
}
void MainWindow::showDatasetInfo(){
    QVector<QStringList> rows;
    int missingTotal=0;
    for(int c=0;c<m_data.columnCount();++c){
        const int missing=m_data.missingCount(c); missingTotal+=missing;
        rows.push_back({m_data.variables()[c].name, variableTypeName(m_data.variables()[c].type), QString::number(m_data.rowCount()), QString::number(missing)});
    }
    showResultTable("Dataset Information", {"Variable","Type","Observations","Missing"}, rows, {2,3},
                    QString("Dataset contains %1 observations, %2 variables and %3 missing cells.").arg(m_data.rowCount()).arg(m_data.columnCount()).arg(missingTotal));
    m_tabs->setCurrentWidget(m_output->parentWidget());
}
void MainWindow::runDescriptiveStatistics(){
    QVector<int> columns;
    for(int c=0;c<m_data.columnCount();++c) if(m_data.variables()[c].type==VariableType::Numeric) columns.push_back(c);
    if(columns.isEmpty()){ QMessageBox::information(this,"Descriptive Statistics","No numeric variables are available. Add or import numeric variables first."); return; }
    auto ranges=m_grid->selectedRanges();
    if(!ranges.isEmpty()){
        QSet<int> selectedCols;
        for(const auto& rg:ranges) for(int c=rg.leftColumn();c<=rg.rightColumn();++c) selectedCols.insert(c);
        QVector<int> chosen; for(int c:selectedCols) if(c>=0 && c<m_data.columnCount() && m_data.variables()[c].type==VariableType::Numeric) chosen.push_back(c);
        if(!chosen.isEmpty()) columns=chosen;
    }
    const auto results=AnalysisEngine::descriptive(m_data,columns);
    QVector<QStringList> rows;
    for(const auto& r:results){
        rows.push_back({r.variable, r.label, QString::number(r.observations), QString::number(r.valid), QString::number(r.blank), QString::number(r.declaredMissing), QString::number(r.nonNumeric),
                        AnalysisEngine::number(r.mean), AnalysisEngine::number(r.stdDev), AnalysisEngine::number(r.variance), AnalysisEngine::number(r.minimum), AnalysisEngine::number(r.q1), AnalysisEngine::number(r.median), AnalysisEngine::number(r.q3), AnalysisEngine::number(r.maximum), AnalysisEngine::number(r.skewness), AnalysisEngine::number(r.kurtosis)});
    }
    showResultTable("Descriptive Statistics",
                    {"Variable","Label","Observations","Valid","Blank","Declared missing","Non-numeric","Mean","Std. Dev.","Variance","Min","Q1","Median","Q3","Max","Skewness","Kurtosis"},
                    rows, {2,3,4,5,6,7,8,9,10,11,12,13,14,15,16},
                    "N is the number of observations in the selected data. Valid values are used for the statistics; blank, declared-missing and non-numeric values remain accounted for.");
    m_tabs->setCurrentWidget(m_output->parentWidget());
    statusBar()->showMessage(QString("Descriptive statistics completed for %1 numeric variable(s)").arg(results.size()),5000);
}


void MainWindow::runFrequencies(){
    if(m_data.columnCount()==0){ QMessageBox::information(this,"Frequencies","Import data or add a variable first."); return; }
    QStringList names; for(const auto& v:m_data.variables()) names << v.name;
    bool ok=false; QString choice=QInputDialog::getItem(this,"Frequencies","Variable:",names,0,false,&ok); if(!ok)return;
    int column=names.indexOf(choice); if(column<0)return;
    const auto results=AnalysisEngine::frequencies(m_data,column);
    const auto summary=AnalysisEngine::frequencySummary(m_data,column);
    QVector<QStringList> rows;
    for(const auto& r:results) rows.push_back({r.value,QString::number(r.count),QString::number(r.percent,'f',2)+"%",r.special?"—":QString::number(r.validPercent,'f',2)+"%",r.special?"—":QString::number(r.cumulativeValidPercent,'f',2)+"%"});
    showResultTable("Frequencies — " + choice, {"Value","Frequency","Percent of observations","Valid percent","Cumulative valid %"}, rows, {1,2,3,4},
                    QString("Observations: %1  |  Valid: %2  |  Blank: %3  |  Declared missing: %4  |  Non-numeric / invalid: %5")
                    .arg(summary.observations).arg(summary.valid).arg(summary.blank).arg(summary.declaredMissing).arg(summary.nonNumeric),
                    "Special values are displayed rather than silently excluded. Valid percent and cumulative valid percent are calculated from valid observations.");
    m_tabs->setCurrentWidget(m_output->parentWidget());
    statusBar()->showMessage(QString("Frequency table completed for '%1'").arg(choice),5000);
}

void MainWindow::runSummaryByGroup(){
    if(m_data.columnCount()<2){ QMessageBox::information(this,"Summary by Group","At least one grouping variable and one numeric variable are required."); return; }
    QStringList numeric, all; for(const auto& v:m_data.variables()){ all<<v.name; if(v.type==VariableType::Numeric) numeric<<v.name; }
    if(numeric.isEmpty()){ QMessageBox::information(this,"Summary by Group","No numeric outcome variables are available."); return; }
    bool ok=false; QString valueName=QInputDialog::getItem(this,"Summary by Group","Numeric variable:",numeric,0,false,&ok); if(!ok)return;
    QString groupName=QInputDialog::getItem(this,"Summary by Group","Group variable:",all,0,false,&ok); if(!ok)return;
    int valueCol=all.indexOf(valueName), groupCol=all.indexOf(groupName); if(valueCol<0||groupCol<0||valueCol==groupCol){QMessageBox::information(this,"Summary by Group","The grouping and numeric variables must be different.");return;}
    const auto results=AnalysisEngine::summaryByGroup(m_data,groupCol,valueCol);
    QVector<QStringList> rows;
    for(const auto& r:results) rows.push_back({r.group,QString::number(r.observations),QString::number(r.valid),QString::number(r.blank),QString::number(r.declaredMissing),QString::number(r.nonNumeric),
                                                AnalysisEngine::number(r.mean),AnalysisEngine::number(r.stdDev),AnalysisEngine::number(r.minimum),AnalysisEngine::number(r.median),AnalysisEngine::number(r.maximum)});
    showResultTable("Summary by Group — " + valueName + " by " + groupName,
                    {"Group","Observations","Valid","Blank","Declared missing","Non-numeric","Mean","Std. Dev.","Min","Median","Max"},
                    rows,{1,2,3,4,5,6,7,8,9,10},
                    "Outcome observations remain accounted for within each group. Group values that are blank, declared-missing or invalid are displayed explicitly.");
    m_tabs->setCurrentWidget(m_output->parentWidget());
    statusBar()->showMessage(QString("Grouped summary completed: %1 by %2").arg(valueName,groupName),5000);
}



static QString inferentialAccounting(const StatPro::ObservationAccounting& a) {
    return QString("Observations: %1  |  Valid: %2  |  Blank: %3  |  Declared missing: %4  |  Non-numeric / invalid: %5")
        .arg(a.observations).arg(a.valid).arg(a.blank).arg(a.declaredMissing).arg(a.nonNumeric);
}

void MainWindow::runPearsonCorrelation(){
    QStringList names; for(const auto& v:m_data.variables()) if(v.type==VariableType::Numeric) names<<v.name;
    if(names.size()<2){QMessageBox::information(this,"Pearson Correlation","At least two numeric variables are required.");return;}
    bool ok=false; QString x=QInputDialog::getItem(this,"Pearson Correlation","Variable X:",names,0,false,&ok);if(!ok)return;
    QString y=QInputDialog::getItem(this,"Pearson Correlation","Variable Y:",names,0,false,&ok);if(!ok)return;if(x==y){QMessageBox::information(this,"Pearson Correlation","Choose two different variables.");return;}
    const int xColumn=m_data.columnNames().indexOf(x); const int yColumn=m_data.columnNames().indexOf(y);
    if(xColumn<0||yColumn<0){showResultMessage("Pearson Correlation","The selected variables could not be found in the dataset.");return;}
    const auto r=AnalysisEngine::pearsonCorrelation(m_data,xColumn,yColumn);
    if(r.pairs<2){showResultMessage("Pearson Correlation","Fewer than 2 complete numeric pairs are available.");return;}
    QString interpretation=std::fabs(r.r)<0.1?"negligible":std::fabs(r.r)<0.3?"weak":std::fabs(r.r)<0.5?"moderate":"strong";
    interpretation += r.r>=0?" positive":" negative";
    QVector<QStringList> rows={{x+" × "+y,QString::number(r.pairs),AnalysisEngine::number(r.r),AnalysisEngine::number(r.p),AnalysisEngine::number(r.ciLow),AnalysisEngine::number(r.ciHigh),interpretation}};
    showResultTable("Pearson Correlation — "+x+" × "+y,{"Variables","Complete pairs","Pearson r","p-value","95% CI low","95% CI high","Interpretation"},rows,{1,2,3,4,5},inferentialAccounting(r),"Pairwise complete numeric observations are used. Missing and invalid values remain accounted for in the observation summary.");
    m_tabs->setCurrentWidget(m_output->parentWidget());statusBar()->showMessage("Pearson correlation completed",5000);
}

void MainWindow::runOneSampleTTest(){
    QStringList names;for(const auto&v:m_data.variables())if(v.type==VariableType::Numeric)names<<v.name;if(names.isEmpty()){QMessageBox::information(this,"One-Sample t Test","No numeric variables are available.");return;}
    bool ok=false;QString name=QInputDialog::getItem(this,"One-Sample t Test","Numeric variable:",names,0,false,&ok);if(!ok)return;
    double mu=QInputDialog::getDouble(this,"One-Sample t Test","Test value (μ₀):",0.0,-1e12,1e12,6,&ok);if(!ok)return;
    const int column=m_data.columnNames().indexOf(name);
    if(column<0){showResultMessage("One-Sample t Test","The selected variable could not be found in the dataset.");return;}
    const auto r=AnalysisEngine::oneSampleTTest(m_data,column,mu);if(r.valid<2){showResultMessage("One-Sample t Test","At least 2 valid numeric observations are required.");return;}
    QVector<QStringList> rows={{name,QString::number(r.valid),AnalysisEngine::number(r.mean),AnalysisEngine::number(r.stdDev),AnalysisEngine::number(r.testMean),AnalysisEngine::number(r.t),AnalysisEngine::number(r.df),AnalysisEngine::number(r.p),AnalysisEngine::number(r.ciLow),AnalysisEngine::number(r.ciHigh),AnalysisEngine::number(r.cohensD)}};
    showResultTable("One-Sample t Test — "+name,{"Variable","Valid N","Mean","Std. Dev.","Test mean","t","df","p-value","95% CI low","95% CI high","Cohen's d"},rows,{1,2,3,4,5,6,7,8,9,10},inferentialAccounting(r),"The confidence interval is for the estimated mean difference (sample mean − test mean). Two-sided p-value; Cohen's d uses the sample standard deviation.");
    m_tabs->setCurrentWidget(m_output->parentWidget());statusBar()->showMessage("One-sample t test completed",5000);
}

void MainWindow::runIndependentTTest(){
    QStringList all,numeric;
    for(const auto&v:m_data.variables()){all<<v.name;if(v.type==VariableType::Numeric)numeric<<v.name;}
    if(numeric.isEmpty()||all.size()<2){QMessageBox::information(this,"Independent-Samples t Test","A numeric outcome and a grouping variable are required.");return;}
    bool ok=false;
    QString value=QInputDialog::getItem(this,"Independent-Samples t Test","Numeric outcome:",numeric,0,false,&ok);
    if(!ok)return;
    const int valueColumn=m_data.columnNames().indexOf(value);
    QStringList groupingNames;
    QMap<QString, QStringList> groupingLevels;
    for(const auto&v:m_data.variables()){
        if(v.name==value) continue;
        const int c=m_data.columnNames().indexOf(v.name);
        const QStringList levels=AnalysisEngine::independentGroupLevels(m_data,c);
        if(levels.size()>=2){ groupingNames<<v.name; groupingLevels.insert(v.name,levels); }
    }
    if(groupingNames.isEmpty()){
        showResultMessage("Independent-Samples t Test","No grouping variable with at least two non-missing groups was found. Select or create a grouping variable with two or more valid groups.");
        return;
    }
    QString group=QInputDialog::getItem(this,"Independent-Samples t Test","Grouping variable:",groupingNames,0,false,&ok);
    if(!ok)return;
    const int groupColumn=m_data.columnNames().indexOf(group);
    const QStringList levels=groupingLevels.value(group);
    QString group1=QInputDialog::getItem(this,"Independent-Samples t Test","Group 1:",levels,0,false,&ok);
    if(!ok)return;
    QStringList remaining=levels; remaining.removeAll(group1);
    QString group2=QInputDialog::getItem(this,"Independent-Samples t Test","Group 2:",remaining,0,false,&ok);
    if(!ok)return;
    QStringList assumptions={"Welch (unequal variances, recommended)","Equal variances assumed"};
    QString assumption=QInputDialog::getItem(this,"Independent-Samples t Test","Variance assumption:",assumptions,0,false,&ok);
    if(!ok)return;
    const auto r=AnalysisEngine::independentTTest(m_data,groupColumn,valueColumn,group1,group2,assumption.startsWith("Equal"));
    if(r.group1.isEmpty()||r.group2.isEmpty()){
        const QString detail=r.availableGroups.isEmpty()?"No valid group levels were found.":QString("Found %1 valid groups: %2. Select any two groups for the comparison.").arg(r.availableGroups.size()).arg(r.availableGroups.join(", "));
        showResultMessage("Independent-Samples t Test",detail);return;
    }
    if(r.n1<2||r.n2<2){
        const QString detail=QString("Each group needs at least 2 valid numeric outcome observations. %1: %2 valid; %3: %4 valid.").arg(r.group1).arg(r.n1).arg(r.group2).arg(r.n2);
        showResultMessage("Independent-Samples t Test",detail);return;
    }
    QVector<QStringList> rows={{r.group1,QString::number(r.n1),AnalysisEngine::number(r.mean1),AnalysisEngine::number(r.sd1),r.group2,QString::number(r.n2),AnalysisEngine::number(r.mean2),AnalysisEngine::number(r.sd2),AnalysisEngine::number(r.difference),AnalysisEngine::number(r.t),AnalysisEngine::number(r.df),AnalysisEngine::number(r.p),AnalysisEngine::number(r.ciLow),AnalysisEngine::number(r.ciHigh),AnalysisEngine::number(r.cohensD)}};
    const QString summary=QString("%1: %2  |  %3: %4").arg(r.group1,inferentialAccounting(r.group1Accounting),r.group2,inferentialAccounting(r.group2Accounting));
    showResultTable("Independent-Samples t Test — "+value+" by "+group,{"Group 1","N1","Mean 1","SD 1","Group 2","N2","Mean 2","SD 2","Mean difference","t","df","p-value","95% CI low","95% CI high","Cohen's d"},rows,{1,2,3,5,6,7,8,9,10,11,12,13,14},summary,assumption+". Difference is Group 1 − Group 2. Missing/invalid outcomes are accounted for separately within each group.");
    m_tabs->setCurrentWidget(m_output->parentWidget());statusBar()->showMessage("Independent-samples t test completed",5000);
}

void MainWindow::runPairedTTest(){
    QStringList names;for(const auto&v:m_data.variables())if(v.type==VariableType::Numeric)names<<v.name;if(names.size()<2){QMessageBox::information(this,"Paired-Samples t Test","At least two numeric variables are required.");return;}
    bool ok=false;QString first=QInputDialog::getItem(this,"Paired-Samples t Test","First measurement:",names,0,false,&ok);if(!ok)return;QString second=QInputDialog::getItem(this,"Paired-Samples t Test","Second measurement:",names,0,false,&ok);if(!ok)return;if(first==second){QMessageBox::information(this,"Paired-Samples t Test","Choose two different measurements.");return;}
    const int firstColumn=m_data.columnNames().indexOf(first); const int secondColumn=m_data.columnNames().indexOf(second);
    if(firstColumn<0||secondColumn<0){showResultMessage("Paired-Samples t Test","The selected variables could not be found in the dataset.");return;}
    const auto r=AnalysisEngine::pairedTTest(m_data,firstColumn,secondColumn);if(r.pairs<2){showResultMessage("Paired-Samples t Test","Fewer than 2 complete numeric pairs are available.");return;}
    QVector<QStringList> rows={{first+" − "+second,QString::number(r.pairs),AnalysisEngine::number(r.meanDifference),AnalysisEngine::number(r.sdDifference),AnalysisEngine::number(r.t),AnalysisEngine::number(r.df),AnalysisEngine::number(r.p),AnalysisEngine::number(r.ciLow),AnalysisEngine::number(r.ciHigh),AnalysisEngine::number(r.cohensDz)}};
    showResultTable("Paired-Samples t Test — "+first+" vs "+second,{"Difference","Complete pairs","Mean difference","SD difference","t","df","p-value","95% CI low","95% CI high","Cohen's dz"},rows,{1,2,3,4,5,6,7,8,9},inferentialAccounting(r),"Pairwise complete observations are used. Difference is first measurement − second measurement; Cohen's dz is based on the standard deviation of paired differences.");
    m_tabs->setCurrentWidget(m_output->parentWidget());statusBar()->showMessage("Paired-samples t test completed",5000);
}

void MainWindow::runChiSquare(){
    if(m_data.columnCount()<2){QMessageBox::information(this,"Chi-Square Test","At least two categorical variables are required.");return;}QStringList names;for(const auto&v:m_data.variables())names<<v.name;bool ok=false;QString row=QInputDialog::getItem(this,"Chi-Square Test of Independence","Row variable:",names,0,false,&ok);if(!ok)return;QString col=QInputDialog::getItem(this,"Chi-Square Test of Independence","Column variable:",names,0,false,&ok);if(!ok)return;if(row==col){QMessageBox::information(this,"Chi-Square Test","Choose two different variables.");return;}
    const auto r=AnalysisEngine::chiSquare(m_data,names.indexOf(row),names.indexOf(col));if(r.rows<2||r.columns<2){showResultMessage("Chi-Square Test of Independence","At least 2 observed categories are required for each variable.");return;}
    QVector<QStringList> table;for(int i=0;i<r.rows;++i)for(int j=0;j<r.columns;++j)table.push_back({r.rowLabels[i],r.columnLabels[j],AnalysisEngine::number(r.observed[i][j]),AnalysisEngine::number(r.expected[i][j])});
    showResultTable("Chi-Square Test of Independence — "+row+" × "+col,{"Row category","Column category","Observed","Expected"},table,{2,3},inferentialAccounting(r),QString("χ² = %1  |  df = %2  |  p-value = %3  |  Cramér's V = %4. Expected counts are based on independence.").arg(AnalysisEngine::number(r.chiSquare)).arg(AnalysisEngine::number(r.df)).arg(AnalysisEngine::number(r.p)).arg(AnalysisEngine::number(r.cramersV)));
    m_tabs->setCurrentWidget(m_output->parentWidget());statusBar()->showMessage("Chi-square test completed",5000);
}

void MainWindow::runOneWayAnova(){
    QStringList all,numeric;for(const auto&v:m_data.variables()){all<<v.name;if(v.type==VariableType::Numeric)numeric<<v.name;}if(numeric.isEmpty()||all.size()<2){QMessageBox::information(this,"One-Way ANOVA","A numeric outcome and grouping variable are required.");return;}bool ok=false;QString value=QInputDialog::getItem(this,"One-Way ANOVA","Numeric outcome:",numeric,0,false,&ok);if(!ok)return;QString group=QInputDialog::getItem(this,"One-Way ANOVA","Grouping variable:",all,0,false,&ok);if(!ok)return;if(value==group){QMessageBox::information(this,"One-Way ANOVA","The outcome and grouping variables must be different.");return;}
    const auto r=AnalysisEngine::oneWayAnova(m_data,all.indexOf(group),all.indexOf(value));if(r.groups<2||r.valid<r.groups){showResultMessage("One-Way ANOVA","At least two groups with valid numeric observations are required.");return;}
    QVector<QStringList> table;for(const auto&g:r.groupStats)table.push_back({"Group: "+g.group,QString::number(g.valid),AnalysisEngine::number(g.mean),AnalysisEngine::number(g.stdDev),"","","","",""});
    table.push_back({"Between groups",QString::number(static_cast<int>(r.dfBetween)),"","",AnalysisEngine::number(r.ssBetween),AnalysisEngine::number(r.msBetween),AnalysisEngine::number(r.f),AnalysisEngine::number(r.p),AnalysisEngine::number(r.etaSquared)});
    table.push_back({"Within groups",QString::number(static_cast<int>(r.dfWithin)),"","",AnalysisEngine::number(r.ssWithin),AnalysisEngine::number(r.msWithin),"","", ""});
    table.push_back({"Total",QString::number(r.valid-1),AnalysisEngine::number(r.grandMean),"",AnalysisEngine::number(r.ssTotal),"","","", ""});
    showResultTable("One-Way ANOVA — "+value+" by "+group,{"Source / group","Valid N","Mean","Std. Dev.","SS","MS","F","p-value","Eta-squared"},table,{1,2,3,4,5,6,7,8},inferentialAccounting(r),"The ANOVA test excludes observations with missing/invalid outcomes or missing/invalid group labels. Group-level counts are shown so excluded observations remain visible in the accounting.");
    m_tabs->setCurrentWidget(m_output->parentWidget());statusBar()->showMessage("One-way ANOVA completed",5000);
}


void MainWindow::runMannWhitneyU(){
    QStringList all,numeric;for(const auto&v:m_data.variables()){all<<v.name;if(v.type==VariableType::Numeric)numeric<<v.name;}if(numeric.isEmpty()||all.size()<2){QMessageBox::information(this,"Mann–Whitney U Test","A numeric outcome and a grouping variable are required.");return;}
    bool ok=false;QString value=QInputDialog::getItem(this,"Mann–Whitney U Test","Numeric outcome:",numeric,0,false,&ok);if(!ok)return;
    QStringList groupingNames;QMap<QString,QStringList> levelsByName;for(const auto&v:m_data.variables()){if(v.name==value)continue;const int c=m_data.columnNames().indexOf(v.name);const auto levels=AnalysisEngine::independentGroupLevels(m_data,c);if(levels.size()>=2){groupingNames<<v.name;levelsByName.insert(v.name,levels);}}
    if(groupingNames.isEmpty()){showResultMessage("Mann–Whitney U Test","No grouping variable with at least two valid groups was found.");return;}
    QString group=QInputDialog::getItem(this,"Mann–Whitney U Test","Grouping variable:",groupingNames,0,false,&ok);if(!ok)return;const int gc=m_data.columnNames().indexOf(group);const auto levels=levelsByName.value(group);QString g1=QInputDialog::getItem(this,"Mann–Whitney U Test","Group 1:",levels,0,false,&ok);if(!ok)return;QStringList remaining=levels;remaining.removeAll(g1);QString g2=QInputDialog::getItem(this,"Mann–Whitney U Test","Group 2:",remaining,0,false,&ok);if(!ok)return;
    const int vc=m_data.columnNames().indexOf(value);const auto r=AnalysisEngine::mannWhitneyU(m_data,gc,vc,g1,g2);if(r.n1<1||r.n2<1){showResultMessage("Mann–Whitney U Test",QString("Each selected group needs at least 1 valid numeric outcome. %1: %2; %3: %4.").arg(g1).arg(r.n1).arg(g2).arg(r.n2));return;}
    QVector<QStringList> table={{g1,QString::number(r.n1),AnalysisEngine::number(r.meanRank1),AnalysisEngine::number(r.u1)},{g2,QString::number(r.n2),AnalysisEngine::number(r.meanRank2),AnalysisEngine::number(r.u2)}};
    const QString summary=QString("U = %1  |  z = %2  |  p-value = %3  |  Effect size r = %4").arg(AnalysisEngine::number(r.statistic)).arg(AnalysisEngine::number(r.z)).arg(AnalysisEngine::number(r.p)).arg(AnalysisEngine::number(r.effectSize));
    showResultTable("Mann–Whitney U Test — "+value+" by "+group,{"Group","N","Mean rank","U"},table,{1,2,3},inferentialAccounting(r),summary+". Two-sided asymptotic normal approximation with tie correction; effect size r = |z|/√N.");m_tabs->setCurrentWidget(m_output->parentWidget());statusBar()->showMessage("Mann–Whitney U test completed",5000);
}

void MainWindow::runWilcoxonSignedRank(){
    QStringList names;for(const auto&v:m_data.variables())if(v.type==VariableType::Numeric)names<<v.name;if(names.size()<2){QMessageBox::information(this,"Wilcoxon Signed-Rank Test","At least two numeric variables are required.");return;}bool ok=false;QString first=QInputDialog::getItem(this,"Wilcoxon Signed-Rank Test","First measurement:",names,0,false,&ok);if(!ok)return;QString second=QInputDialog::getItem(this,"Wilcoxon Signed-Rank Test","Second measurement:",names,0,false,&ok);if(!ok)return;if(first==second){QMessageBox::information(this,"Wilcoxon Signed-Rank Test","Choose two different measurements.");return;}
    const auto r=AnalysisEngine::wilcoxonSignedRank(m_data,m_data.columnNames().indexOf(first),m_data.columnNames().indexOf(second));if(r.pairs<2){showResultMessage("Wilcoxon Signed-Rank Test","At least 2 non-zero complete paired differences are required.");return;}
    QVector<QStringList> table={{first+" − "+second,QString::number(r.pairs),AnalysisEngine::number(r.wPlus),AnalysisEngine::number(r.wMinus),AnalysisEngine::number(r.statistic),AnalysisEngine::number(r.z),AnalysisEngine::number(r.p),AnalysisEngine::number(r.effectSize)}};
    showResultTable("Wilcoxon Signed-Rank Test — "+first+" vs "+second,{"Comparison","N","Positive rank sum","Negative rank sum","W","z","p-value","Effect size r"},table,{1,2,3,4,5,6,7},inferentialAccounting(r),"Two-sided asymptotic normal approximation with tie correction and continuity correction. Zero differences are omitted from the signed-rank calculation; effect size r = |z|/√N.");m_tabs->setCurrentWidget(m_output->parentWidget());statusBar()->showMessage("Wilcoxon signed-rank test completed",5000);
}

void MainWindow::runKruskalWallis(){
    QStringList all,numeric;for(const auto&v:m_data.variables()){all<<v.name;if(v.type==VariableType::Numeric)numeric<<v.name;}if(numeric.isEmpty()||all.size()<2){QMessageBox::information(this,"Kruskal–Wallis Test","A numeric outcome and grouping variable are required.");return;}bool ok=false;QString value=QInputDialog::getItem(this,"Kruskal–Wallis Test","Numeric outcome:",numeric,0,false,&ok);if(!ok)return;QStringList groupingNames;for(const auto&v:m_data.variables())if(v.name!=value){const auto levels=AnalysisEngine::independentGroupLevels(m_data,m_data.columnNames().indexOf(v.name));if(levels.size()>=2)groupingNames<<v.name;}if(groupingNames.isEmpty()){showResultMessage("Kruskal–Wallis Test","No grouping variable with at least two valid groups was found.");return;}QString group=QInputDialog::getItem(this,"Kruskal–Wallis Test","Grouping variable:",groupingNames,0,false,&ok);if(!ok)return;
    const auto r=AnalysisEngine::kruskalWallis(m_data,m_data.columnNames().indexOf(group),m_data.columnNames().indexOf(value));if(r.groups<2){showResultMessage("Kruskal–Wallis Test","At least two groups with valid numeric observations are required.");return;}
    QVector<QStringList> table;for(int i=0;i<r.groupLabels.size();++i)table.push_back({r.groupLabels[i],QString::number(r.groupNs[i]),AnalysisEngine::number(r.groupMeanRanks[i])});
    const QString summary=QString("H = %1  |  df = %2  |  p-value = %3  |  Epsilon-squared = %4").arg(AnalysisEngine::number(r.statistic)).arg(AnalysisEngine::number(r.z)).arg(AnalysisEngine::number(r.p)).arg(AnalysisEngine::number(r.effectSize));
    showResultTable("Kruskal–Wallis Test — "+value+" by "+group,{"Group","N","Mean rank"},table,{1,2},inferentialAccounting(r),summary+". Tie-corrected chi-square approximation; epsilon-squared is reported as an omnibus effect-size estimate. A significant result indicates that at least one group distribution/rank location differs, but does not identify which groups differ.");m_tabs->setCurrentWidget(m_output->parentWidget());statusBar()->showMessage("Kruskal–Wallis test completed",5000);
}

void MainWindow::runSpearmanCorrelation(){
    QStringList names;for(const auto&v:m_data.variables())if(v.type==VariableType::Numeric)names<<v.name;if(names.size()<2){QMessageBox::information(this,"Spearman Rank Correlation","At least two numeric variables are required.");return;}bool ok=false;QString x=QInputDialog::getItem(this,"Spearman Rank Correlation","Variable X:",names,0,false,&ok);if(!ok)return;QString y=QInputDialog::getItem(this,"Spearman Rank Correlation","Variable Y:",names,0,false,&ok);if(!ok)return;if(x==y){QMessageBox::information(this,"Spearman Rank Correlation","Choose two different variables.");return;}
    const auto r=AnalysisEngine::spearmanCorrelation(m_data,m_data.columnNames().indexOf(x),m_data.columnNames().indexOf(y));if(r.pairs<3){showResultMessage("Spearman Rank Correlation","At least 3 complete numeric pairs are required.");return;}QString interpretation=std::fabs(r.rho)<0.1?"negligible":std::fabs(r.rho)<0.3?"weak":std::fabs(r.rho)<0.5?"moderate":"strong";interpretation+=r.rho>=0?" positive":" negative";
    QVector<QStringList> table={{x+" × "+y,QString::number(r.pairs),AnalysisEngine::number(r.rho),AnalysisEngine::number(r.t),AnalysisEngine::number(r.df),AnalysisEngine::number(r.p),interpretation}};showResultTable("Spearman Rank Correlation — "+x+" × "+y,{"Variables","Complete pairs","Spearman rho","t","df","p-value","Interpretation"},table,{1,2,3,4,5},inferentialAccounting(r),"Spearman correlation is computed as Pearson correlation on ranked values, with average ranks for ties. The p-value uses a Student-t approximation with n−2 degrees of freedom.");m_tabs->setCurrentWidget(m_output->parentWidget());statusBar()->showMessage("Spearman correlation completed",5000);
}

void MainWindow::runSimpleLinearRegression(){
    QStringList numeric;
    for(const auto& v:m_data.variables()) if(v.type==VariableType::Numeric) numeric<<v.name;
    if(numeric.size()<2){QMessageBox::information(this,"Simple Linear Regression","At least two numeric variables are required: one predictor (X) and one outcome (Y).");return;}
    bool ok=false;
    QString y=QInputDialog::getItem(this,"Simple Linear Regression","Outcome variable (Y):",numeric,0,false,&ok);if(!ok)return;
    QString x=QInputDialog::getItem(this,"Simple Linear Regression","Predictor variable (X):",numeric,0,false,&ok);if(!ok)return;
    if(x==y){QMessageBox::information(this,"Simple Linear Regression","Choose different outcome and predictor variables.");return;}
    const int xColumn=m_data.columnNames().indexOf(x);
    const int yColumn=m_data.columnNames().indexOf(y);
    const auto r=AnalysisEngine::simpleLinearRegression(m_data,xColumn,yColumn);
    if(r.complete<3){showResultMessage("Simple Linear Regression","At least 3 complete numeric observations are required. Observations with blank, declared-missing or invalid X/Y values cannot be used in the fitted model.");return;}
    const QString interpretation = r.pSlope<0.05
        ? QString("The estimated slope is statistically significant at α = 0.05; a one-unit increase in %1 is associated with an estimated %2-unit change in %3.").arg(x,AnalysisEngine::number(r.slope),y)
        : QString("The estimated slope is not statistically significant at α = 0.05; the data do not provide strong evidence of a linear association between %1 and %2.").arg(x,y);
    QVector<QStringList> table={
        {"Intercept",AnalysisEngine::number(r.intercept),AnalysisEngine::number(r.seIntercept),AnalysisEngine::number(r.tIntercept),AnalysisEngine::number(r.pIntercept),AnalysisEngine::number(r.interceptCiLow),AnalysisEngine::number(r.interceptCiHigh)},
        {x,AnalysisEngine::number(r.slope),AnalysisEngine::number(r.seSlope),AnalysisEngine::number(r.tSlope),AnalysisEngine::number(r.pSlope),AnalysisEngine::number(r.slopeCiLow),AnalysisEngine::number(r.slopeCiHigh)}
    };
    const QString summary=QString("Complete N: %1  |  R²: %2  |  Adjusted R²: %3  |  RMSE: %4  |  F(1,%5): %6  |  Model p-value: %7  |  Durbin–Watson: %8")
        .arg(r.complete).arg(AnalysisEngine::number(r.rSquared)).arg(AnalysisEngine::number(r.adjustedRSquared)).arg(AnalysisEngine::number(r.rmse))
        .arg(static_cast<int>(r.dfResidual)).arg(AnalysisEngine::number(r.f)).arg(AnalysisEngine::number(r.fP)).arg(AnalysisEngine::number(r.durbinWatson));
    const QString note=QString("Model: %1 = %2 + (%3 × %4). %5 Observation accounting — total observations: %6; complete: %7; X blank: %8; Y blank: %9; X declared missing: %10; Y declared missing: %11; X non-numeric/invalid: %12; Y non-numeric/invalid: %13.")
        .arg(y,AnalysisEngine::number(r.intercept),AnalysisEngine::number(r.slope),x,interpretation)
        .arg(r.observations).arg(r.complete).arg(r.xBlank).arg(r.yBlank).arg(r.xDeclaredMissing).arg(r.yDeclaredMissing).arg(r.xNonNumeric).arg(r.yNonNumeric);
    showResultTable("Simple Linear Regression — "+y+" on "+x,{"Term","Estimate","Std. Error","t","p-value","95% CI low","95% CI high"},table,{1,2,3,4,5,6},note,summary);
    m_tabs->setCurrentWidget(m_output->parentWidget());statusBar()->showMessage("Simple linear regression completed",5000);
}


void MainWindow::runMultipleLinearRegression(){
    QStringList numeric;
    for(const auto& v:m_data.variables()) if(v.type==VariableType::Numeric) numeric<<v.name;
    if(numeric.size()<2){QMessageBox::information(this,"Multiple Linear Regression","At least two numeric variables are required.");return;}

    QDialog dialog(this); dialog.setWindowTitle("Multiple Linear Regression"); dialog.resize(520,500);
    auto* layout=new QVBoxLayout(&dialog);
    auto* form=new QFormLayout;
    auto* outcome=new QComboBox; outcome->addItems(numeric);
    form->addRow("Outcome variable (Y):",outcome);
    layout->addLayout(form);
    layout->addWidget(new QLabel("Predictors (select one or more):"));
    auto* predictors=new QListWidget;
    predictors->setSelectionMode(QAbstractItemView::ExtendedSelection);
    predictors->addItems(numeric);
    layout->addWidget(predictors,1);
    auto* hint=new QLabel("Tip: hold Ctrl to select multiple predictors. The outcome variable cannot also be a predictor.");
    hint->setWordWrap(true); layout->addWidget(hint);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()!=QDialog::Accepted)return;

    const QString yName=outcome->currentText();
    QStringList predictorNames;
    for(auto* item:predictors->selectedItems()) predictorNames<<item->text();
    predictorNames.removeAll(yName);
    predictorNames.removeDuplicates();
    if(predictorNames.isEmpty()){QMessageBox::information(this,"Multiple Linear Regression","Select at least one predictor different from the outcome variable.");return;}

    const int yColumn=m_data.columnNames().indexOf(yName);
    QVector<int> predictorColumns;
    for(const QString& name:predictorNames){const int c=m_data.columnNames().indexOf(name);if(c>=0)predictorColumns.push_back(c);}
    if(yColumn<0 || predictorColumns.size()!=predictorNames.size()){showResultMessage("Multiple Linear Regression","One or more selected variables could not be found in the dataset.");return;}

    const auto r=AnalysisEngine::multipleLinearRegression(m_data,predictorColumns,yColumn);
    if(r.complete<=r.predictors+1){
        showResultMessage("Multiple Linear Regression",QString("Insufficient complete observations. This model has %1 predictor(s) and requires more than %2 complete numeric observations; only %3 are available.").arg(r.predictors).arg(r.predictors+1).arg(r.complete));
        return;
    }
    if(r.singular){
        showResultMessage("Multiple Linear Regression","The model cannot be estimated because the predictors are perfectly collinear or otherwise form a singular design matrix. Remove a redundant predictor and try again.");
        return;
    }
    if(r.coefficients.isEmpty()){showResultMessage("Multiple Linear Regression","The model could not be estimated from the selected variables.");return;}

    QVector<QStringList> table;
    for(const auto& c:r.coefficients){
        table.push_back({c.term,AnalysisEngine::number(c.estimate),AnalysisEngine::number(c.stdError),AnalysisEngine::number(c.standardizedBeta),AnalysisEngine::number(c.t),AnalysisEngine::number(c.p),AnalysisEngine::number(c.ciLow),AnalysisEngine::number(c.ciHigh),AnalysisEngine::number(c.vif)});
    }
    const QString summary=QString("Complete N: %1  |  Predictors: %2  |  R²: %3  |  Adjusted R²: %4  |  RMSE: %5  |  F(%6,%7): %8  |  Model p-value: %9  |  Durbin–Watson: %10")
        .arg(r.complete).arg(r.predictors).arg(AnalysisEngine::number(r.rSquared)).arg(AnalysisEngine::number(r.adjustedRSquared)).arg(AnalysisEngine::number(r.rmse))
        .arg(static_cast<int>(r.dfRegression)).arg(static_cast<int>(r.dfResidual)).arg(AnalysisEngine::number(r.f)).arg(AnalysisEngine::number(r.fP)).arg(AnalysisEngine::number(r.durbinWatson));
    QString equation=yName+" = "+AnalysisEngine::number(r.coefficients[0].estimate);
    for(int i=1;i<r.coefficients.size();++i) equation+=QString(" + (%1 × %2)").arg(AnalysisEngine::number(r.coefficients[i].estimate),r.coefficients[i].term);
    const QString note=QString("Model: %1. Observation accounting — total: %2; complete cases used: %3; excluded due to blank values: %4; excluded due to declared missing values: %5; excluded due to non-numeric/invalid values: %6. Standardized beta is reported for predictors. VIF is a multicollinearity diagnostic; values above about 5 merit attention and values above 10 are commonly treated as serious.")
        .arg(equation).arg(r.observations).arg(r.complete).arg(r.excludedBlank).arg(r.excludedDeclaredMissing).arg(r.excludedNonNumeric);
    showResultTable("Multiple Linear Regression — "+yName,{"Term","Estimate","Std. Error","Std. Beta","t","p-value","95% CI low","95% CI high","VIF"},table,{1,2,3,4,5,6,7,8},note,summary);
    m_tabs->setCurrentWidget(m_output->parentWidget());
    statusBar()->showMessage("Multiple linear regression completed",5000);
}

void MainWindow::undo(){m_undoStack->undo();m_state.setDirty(true);refreshDataView();}
void MainWindow::redo(){m_undoStack->redo();m_state.setDirty(true);refreshDataView();}

void MainWindow::applyFilter(){const QString q=m_filterEdit->text().trimmed();QVector<int> rows=m_data.filteredRows(q);refreshDataView(rows);statusBar()->showMessage(QString("%1 of %2 rows shown").arg(rows.size()).arg(m_data.rowCount()));}
void MainWindow::clearFilter(){m_filterEdit->clear();refreshDataView();}
void MainWindow::replaceText(){bool ok=false;const auto find=QInputDialog::getText(this,"Find / Replace","Find:",QLineEdit::Normal,"",&ok);if(!ok||find.isEmpty())return;const auto repl=QInputDialog::getText(this,"Find / Replace","Replace with:",QLineEdit::Normal,"",&ok);if(!ok)return;int count=0;for(int r=0;r<m_data.rowCount();++r)for(int c=0;c<m_data.columnCount();++c){auto s=m_data.value(r,c).toString();if(s.contains(find,Qt::CaseInsensitive)){s.replace(find,repl,Qt::CaseInsensitive);QString err;if(m_data.validateValue(c,s,&err)) {m_data.setValue(r,c,s);++count;}}}if(count){m_state.setDirty(true);refreshDataView();}showResultMessage("Find / Replace", QString("Find / Replace complete. %1 cells changed.").arg(count));}
void MainWindow::showResultMessage(const QString& title, const QString& message){
    showResultTable(title, {"Output"}, {{message}}, {}, QString());
    m_tabs->setCurrentWidget(m_output->parentWidget());
}

void MainWindow::showResultTable(const QString& title, const QStringList& headers, const QVector<QStringList>& rows,
                                 const QSet<int>& numericColumns, const QString& note, const QString& summary){
    m_outputTitle->setText(title);
    m_outputSummary->setText(summary);
    m_outputSummary->setVisible(!summary.isEmpty());
    m_outputNote->setText(note);
    m_outputNote->setVisible(!note.isEmpty());
    m_output->clear();
    m_output->setColumnCount(headers.size());
    m_output->setRowCount(rows.size());
    m_output->setHorizontalHeaderLabels(headers);
    for(int r=0;r<rows.size();++r){
        for(int c=0;c<headers.size();++c){
            QString text=c<rows[r].size()?rows[r][c]:QString();
            if(numericColumns.contains(c)) {
                const int decimals=QSettings("StatPro Analytics","StatPro Analytics").value("results/decimals",4).toInt();
                QString raw=text.endsWith('%') ? text.left(text.size()-1) : text;
                bool ok=false; const double numeric=raw.remove(',').toDouble(&ok);
                if(ok) text=QString::number(numeric,'f',decimals)+(text.endsWith('%')?"%":"");
            }
            auto* item=new QTableWidgetItem(text);
            const bool numeric=numericColumns.contains(c);
            item->setTextAlignment(numeric?Qt::AlignRight|Qt::AlignVCenter:Qt::AlignLeft|Qt::AlignVCenter);
            if(numeric) {
                QString raw=text.endsWith('%') ? text.left(text.size()-1) : text;
                bool ok=false; const double numericValue=raw.remove(',').toDouble(&ok);
                if(ok) { item->setData(Qt::UserRole,numericValue); item->setData(Qt::UserRole+1,text.endsWith('%')); }
            }
            m_output->setItem(r,c,item);
        }
    }
    m_output->resizeColumnsToContents();
    for(int c=0;c<m_output->columnCount();++c){
        int width=m_output->columnWidth(c);
        m_output->setColumnWidth(c,qBound(90,width+18,360));
    }
    m_output->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    applyResultsFormatting();
}

void MainWindow::runRegressionWithCategoricalPredictors(){
    QStringList outcomes;
    QStringList predictors;
    for(const auto& v:m_data.variables()){
        if(v.type==VariableType::Numeric) outcomes<<v.name;
        if(v.type==VariableType::Numeric || v.type==VariableType::String || v.type==VariableType::Boolean) predictors<<v.name;
    }
    if(outcomes.isEmpty() || predictors.size()<2){
        QMessageBox::information(this,"Multiple Linear Regression with Categorical Predictors","A numeric outcome and at least one predictor are required.");
        return;
    }
    QDialog dialog(this); dialog.setWindowTitle("Multiple Linear Regression with Categorical Predictors"); dialog.resize(620,560);
    auto* layout=new QVBoxLayout(&dialog);
    auto* form=new QFormLayout; auto* outcome=new QComboBox; outcome->addItems(outcomes);
    form->addRow("Numeric outcome (Y):",outcome); layout->addLayout(form);
    layout->addWidget(new QLabel("Predictors (numeric = continuous; text/Boolean = categorical):"));
    auto* list=new QListWidget; list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    for(const auto& name:predictors){
        const int c=m_data.columnNames().indexOf(name); const auto type=m_data.variables()[c].type;
        auto* item=new QListWidgetItem(name+"  ["+variableTypeName(type)+"]"); item->setData(Qt::UserRole,name); list->addItem(item);
    }
    layout->addWidget(list,1);
    auto* hint=new QLabel("Categorical predictors use reference-cell (dummy) coding. The first sorted valid level is the reference; each other level is compared with it. Numeric predictors remain continuous.");
    hint->setWordWrap(true); layout->addWidget(hint);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel); layout->addWidget(buttons);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()!=QDialog::Accepted)return;
    const QString yName=outcome->currentText();
    QStringList predictorNames;
    for(auto* item:list->selectedItems()) predictorNames<<item->data(Qt::UserRole).toString();
    predictorNames.removeAll(yName); predictorNames.removeDuplicates();
    if(predictorNames.isEmpty()){
        QMessageBox::information(this,"Multiple Linear Regression with Categorical Predictors","Select at least one predictor different from the outcome variable.");
        return;
    }
    const int yColumn=m_data.columnNames().indexOf(yName); QVector<int> predictorColumns;
    for(const QString& name:predictorNames){const int c=m_data.columnNames().indexOf(name);if(c>=0)predictorColumns.push_back(c);}
    if(yColumn<0 || predictorColumns.size()!=predictorNames.size()){
        showResultMessage("Categorical Regression","One or more selected variables could not be found in the dataset."); return;
    }
    const auto r=AnalysisEngine::regressionWithCategoricalPredictors(m_data,predictorColumns,yColumn);
    if(r.coefficients.isEmpty()){
        showResultMessage("Categorical Regression","The model could not be estimated. Check that the selected predictors have variation and that enough complete observations are available."); return;
    }
    if(r.singular){
        showResultMessage("Categorical Regression","The model cannot be estimated because the design matrix is singular. This can happen when a predictor has no variation, when predictors are redundant, or when there are too few complete observations. Remove a redundant predictor or combine sparse categories and try again."); return;
    }
    const int modelParameters=r.coefficients.size();
    if(r.complete<=modelParameters){
        showResultMessage("Categorical Regression",QString("Insufficient complete observations. The expanded model has %1 parameter(s) and only %2 complete observations. More complete observations are required.").arg(modelParameters).arg(r.complete)); return;
    }
    QVector<QStringList> table; table.reserve(r.coefficients.size());
    for(const auto& c:r.coefficients){
        table.push_back({c.term,AnalysisEngine::number(c.estimate),AnalysisEngine::number(c.stdError),AnalysisEngine::number(c.standardizedBeta),AnalysisEngine::number(c.t),AnalysisEngine::number(c.p),AnalysisEngine::number(c.ciLow),AnalysisEngine::number(c.ciHigh),AnalysisEngine::number(c.vif)});
    }
    const QString summary=QString("Complete N: %1  |  Original predictors: %2  |  Model parameters: %3  |  R²: %4  |  Adjusted R²: %5  |  RMSE: %6  |  F(%7,%8): %9  |  Model p-value: %10  |  Durbin–Watson: %11")
        .arg(r.complete).arg(r.predictors).arg(r.coefficients.size()).arg(AnalysisEngine::number(r.rSquared)).arg(AnalysisEngine::number(r.adjustedRSquared)).arg(AnalysisEngine::number(r.rmse))
        .arg(static_cast<int>(r.dfRegression)).arg(static_cast<int>(r.dfResidual)).arg(AnalysisEngine::number(r.f)).arg(AnalysisEngine::number(r.fP)).arg(AnalysisEngine::number(r.durbinWatson));
    const QString note=QString("Categorical predictors use reference-cell coding. For each categorical predictor, the first sorted valid level is the reference and each displayed level coefficient is the adjusted difference from that reference, holding other predictors constant. Numeric predictors are treated as continuous. Complete-case accounting — total observations: %1; complete: %2; excluded because of blank values: %3; declared missing: %4; non-numeric/invalid: %5.")
        .arg(r.observations).arg(r.complete).arg(r.excludedBlank).arg(r.excludedDeclaredMissing).arg(r.excludedNonNumeric);
    showResultTable("Multiple Linear Regression with Categorical Predictors — "+yName,{"Term","Estimate","Std. Error","Std. Beta","t","p-value","95% CI low","95% CI high","VIF"},table,{1,2,3,4,5,6,7,8},note,summary);
    m_tabs->setCurrentWidget(m_output->parentWidget()); statusBar()->showMessage("Categorical regression completed",5000);
}


void MainWindow::runRegressionDiagnostics(){
    QStringList numeric;
    for(const auto& v:m_data.variables()) if(v.type==VariableType::Numeric) numeric<<v.name;
    if(numeric.size()<2){QMessageBox::information(this,"Regression Diagnostics","At least two numeric variables are required: one outcome and at least one predictor.");return;}
    QDialog dialog(this); dialog.setWindowTitle("Regression Diagnostics"); dialog.resize(560,520);
    auto* layout=new QVBoxLayout(&dialog);
    auto* form=new QFormLayout; auto* outcome=new QComboBox; outcome->addItems(numeric);
    form->addRow("Outcome variable (Y):",outcome); layout->addLayout(form);
    layout->addWidget(new QLabel("Predictors (select one or more):"));
    auto* predictors=new QListWidget; predictors->setSelectionMode(QAbstractItemView::ExtendedSelection); predictors->addItems(numeric); layout->addWidget(predictors,1);
    auto* hint=new QLabel("Diagnostics use the same complete-case OLS model as Multiple Linear Regression. Hold Ctrl to select multiple predictors."); hint->setWordWrap(true); layout->addWidget(hint);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel); layout->addWidget(buttons);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept); connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()!=QDialog::Accepted)return;
    const QString yName=outcome->currentText(); QStringList predictorNames; for(auto* item:predictors->selectedItems()) predictorNames<<item->text();
    predictorNames.removeAll(yName); predictorNames.removeDuplicates();
    if(predictorNames.isEmpty()){QMessageBox::information(this,"Regression Diagnostics","Select at least one predictor different from the outcome variable.");return;}
    const int yColumn=m_data.columnNames().indexOf(yName); QVector<int> predictorColumns;
    for(const QString& name:predictorNames){const int c=m_data.columnNames().indexOf(name);if(c>=0)predictorColumns.push_back(c);}
    if(yColumn<0 || predictorColumns.size()!=predictorNames.size()){showResultMessage("Regression Diagnostics","One or more selected variables could not be found in the dataset.");return;}
    const auto r=AnalysisEngine::regressionDiagnostics(m_data,predictorColumns,yColumn);
    if(r.complete<=r.parameters){showResultMessage("Regression Diagnostics",QString("Insufficient complete observations. The model has %1 parameters and requires more than %1 complete numeric observations; only %2 are available.").arg(r.parameters).arg(r.complete));return;}
    if(r.singular){showResultMessage("Regression Diagnostics","The diagnostic model cannot be estimated because the predictors are perfectly collinear or otherwise form a singular design matrix. Remove a redundant predictor and try again.");return;}
    if(r.rows.isEmpty()){showResultMessage("Regression Diagnostics","No complete observations were available for diagnostics.");return;}
    QVector<QStringList> table; table.reserve(r.rows.size());
    for(const auto& d:r.rows){QStringList flags; if(d.highLeverage)flags<<"High leverage"; if(d.influential)flags<<"Influential (Cook's D)"; if(d.largeResidual)flags<<"Large residual"; table.push_back({QString::number(d.observation),AnalysisEngine::number(d.actual),AnalysisEngine::number(d.predicted),AnalysisEngine::number(d.residual),AnalysisEngine::number(d.standardizedResidual),AnalysisEngine::number(d.studentizedResidual),AnalysisEngine::number(d.leverage),AnalysisEngine::number(d.cooksDistance),flags.join("; ")});}
    const QString summary=QString("Complete N: %1  |  Predictors: %2  |  RMSE: %3  |  R²: %4  |  Adjusted R²: %5  |  Durbin–Watson: %6  |  Jarque–Bera p: %7")
        .arg(r.complete).arg(r.predictors).arg(AnalysisEngine::number(r.rmse)).arg(AnalysisEngine::number(r.rSquared)).arg(AnalysisEngine::number(r.adjustedRSquared)).arg(AnalysisEngine::number(r.durbinWatson)).arg(AnalysisEngine::number(r.jarqueBeraP));
    const QString note=QString("Observation-level OLS diagnostics for %1. High leverage is flagged when leverage > 2p/n; influential observations when Cook's distance > 4/n; large residuals when |externally studentized residual| > 2. Counts: high leverage = %2, influential = %3, large residual = %4. Maximum leverage = %5; maximum Cook's distance = %6. Residual normality is summarized by the Jarque–Bera test (p < 0.05 indicates evidence against normality). Excluded observations — blank: %7; declared missing: %8; non-numeric/invalid: %9.")
        .arg(yName).arg(r.highLeverageCount).arg(r.influentialCount).arg(r.largeResidualCount).arg(AnalysisEngine::number(r.maxLeverage)).arg(AnalysisEngine::number(r.maxCooksDistance)).arg(r.excludedBlank).arg(r.excludedDeclaredMissing).arg(r.excludedNonNumeric);
    showResultTable("Regression Diagnostics — "+yName,{"Observation","Actual","Predicted","Residual","Std. residual","Studentized residual","Leverage","Cook's D","Flag"},table,{1,2,3,4,5,6,7},note,summary);
    m_tabs->setCurrentWidget(m_output->parentWidget()); statusBar()->showMessage("Regression diagnostics completed",5000);
}


void MainWindow::runRegressionPrediction(){
    QStringList numeric; for(const auto& v:m_data.variables()) if(v.type==VariableType::Numeric) numeric<<v.name;
    if(numeric.size()<2){QMessageBox::information(this,"Regression Prediction","At least two numeric variables are required.");return;}
    QDialog d(this);d.setWindowTitle("Regression Prediction & Fitted Values");d.resize(520,360);auto* f=new QFormLayout(&d);auto* y=new QComboBox;auto* x=new QComboBox;y->addItems(numeric);x->addItems(numeric);x->setCurrentIndex(numeric.size()>1?1:0);auto* w=new QSpinBox;w->setRange(1,100);w->setValue(3);f->addRow("Outcome variable (Y):",y);f->addRow("Predictor variable (X):",x);f->addRow("Moving-average window (display):",w);auto* info=new QLabel("Produces observed fitted values, residuals, 95% confidence intervals for the mean response, and 95% prediction intervals for each complete observation.");info->setWordWrap(true);f->addRow(info);auto* b=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);f->addRow(b);connect(b,&QDialogButtonBox::accepted,&d,&QDialog::accept);connect(b,&QDialogButtonBox::rejected,&d,&QDialog::reject);if(d.exec()!=QDialog::Accepted)return;if(x->currentText()==y->currentText()){QMessageBox::information(this,"Regression Prediction","Choose different X and Y variables.");return;}
    const int xc=m_data.columnNames().indexOf(x->currentText()),yc=m_data.columnNames().indexOf(y->currentText());const auto r=AnalysisEngine::regressionPrediction(m_data,xc,yc,w->value());if(r.complete<3){showResultMessage("Regression Prediction","At least 3 complete numeric X/Y observations are required.");return;}
    QVector<QStringList> table;for(const auto& z:r.rows)table.push_back({QString::number(z.observation),AnalysisEngine::number(z.x),AnalysisEngine::number(z.actual),AnalysisEngine::number(z.fitted),AnalysisEngine::number(z.residual),AnalysisEngine::number(z.meanCiLow),AnalysisEngine::number(z.meanCiHigh),AnalysisEngine::number(z.predictionLow),AnalysisEngine::number(z.predictionHigh)});
    const QString summary=QString("Complete N: %1  |  R²: %2  |  RMSE: %3  |  Model: Y = %4 + %5X").arg(r.complete).arg(AnalysisEngine::number(r.rSquared)).arg(AnalysisEngine::number(r.rmse)).arg(AnalysisEngine::number(r.intercept)).arg(AnalysisEngine::number(r.slope));
    const QString note="Fitted values and residuals are based on complete numeric X/Y observations. The mean-response interval estimates the uncertainty around the fitted mean; the prediction interval is wider and is for an individual future/observed response. Intervals use the t critical value with n−2 residual degrees of freedom.";
    showResultTable("Regression Prediction — "+y->currentText()+" on "+x->currentText(),{"Observation","X","Actual Y","Fitted Y","Residual","Mean CI low","Mean CI high","Prediction low","Prediction high"},table,{1,2,3,4,5,6,7,8},note,summary);m_tabs->setCurrentWidget(m_output->parentWidget());statusBar()->showMessage("Regression predictions completed",5000);
}

void MainWindow::runLogisticRegression(){
    QStringList outcomes;for(const auto& v:m_data.variables())if(v.type==VariableType::Numeric||v.type==VariableType::String||v.type==VariableType::Boolean)outcomes<<v.name;QStringList numeric;for(const auto& v:m_data.variables())if(v.type==VariableType::Numeric)numeric<<v.name;if(outcomes.isEmpty()||numeric.isEmpty()){QMessageBox::information(this,"Logistic Regression","A binary outcome and at least one numeric predictor are required.");return;}
    QDialog d(this);d.setWindowTitle("Binary Logistic Regression");d.resize(600,520);auto* l=new QVBoxLayout(&d);auto* form=new QFormLayout;auto* y=new QComboBox;y->addItems(outcomes);form->addRow("Binary outcome:",y);l->addLayout(form);l->addWidget(new QLabel("Numeric predictors (Ctrl-click for multiple):"));auto* pred=new QListWidget;pred->setSelectionMode(QAbstractItemView::ExtendedSelection);pred->addItems(numeric);l->addWidget(pred,1);auto* hint=new QLabel("The outcome must have exactly two non-missing levels. Numeric outcomes may use any two distinct values; text/Boolean outcomes may use any two labels. The lower/sorted level is coded 0 and the higher/sorted level is coded 1, with common yes/no-style labels mapped semantically. Predictors are numeric only.");hint->setWordWrap(true);l->addWidget(hint);auto* b=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);l->addWidget(b);connect(b,&QDialogButtonBox::accepted,&d,&QDialog::accept);connect(b,&QDialogButtonBox::rejected,&d,&QDialog::reject);if(d.exec()!=QDialog::Accepted)return;QStringList pn;for(auto* it:pred->selectedItems())pn<<it->text();pn.removeAll(y->currentText());pn.removeDuplicates();if(pn.isEmpty()){QMessageBox::information(this,"Logistic Regression","Select at least one predictor different from the outcome.");return;}int yc=m_data.columnNames().indexOf(y->currentText());QVector<int> pc;for(const auto& n:pn)pc<<m_data.columnNames().indexOf(n);const auto r=AnalysisEngine::logisticRegression(m_data,yc,pc);if(r.outcomeLevels!=2){showResultMessage("Logistic Regression",QString("The selected outcome is not binary. It contains %1 distinct non-missing levels; exactly 2 are required.").arg(r.outcomeLevels));return;}if(r.complete<=r.parameters){showResultMessage("Logistic Regression",QString("Insufficient complete observations. %1 complete observations are available for a model with %2 parameters.").arg(r.complete).arg(r.parameters));return;}if(r.singular){showResultMessage("Logistic Regression","The model design is singular. Remove redundant or constant predictors.");return;}if(!r.converged){showResultMessage("Logistic Regression","The maximum-likelihood algorithm did not converge. This can occur with complete/quasi-complete separation or extreme predictor values. Try removing a sparse predictor or rescaling the data.");return;}
    QVector<QStringList> table;for(const auto& c:r.coefficients)table.push_back({c.term,AnalysisEngine::number(c.estimate),AnalysisEngine::number(c.stdError),AnalysisEngine::number(c.z),AnalysisEngine::number(c.p),AnalysisEngine::number(c.oddsRatio),AnalysisEngine::number(c.ciLow),AnalysisEngine::number(c.ciHigh)});
    const QString summary=QString("Complete N: %1  |  −2 Log Likelihood: %2  |  AIC: %3  |  BIC: %4  |  McFadden R²: %5  |  Accuracy: %6%%  |  Sensitivity: %7%%  |  Specificity: %8%%  |  Iterations: %9").arg(r.complete).arg(AnalysisEngine::number(r.minus2LogLikelihood)).arg(AnalysisEngine::number(r.aic)).arg(AnalysisEngine::number(r.bic)).arg(AnalysisEngine::number(r.mcfaddenR2)).arg(AnalysisEngine::number(100*r.accuracy)).arg(AnalysisEngine::number(100*r.sensitivity)).arg(AnalysisEngine::number(100*r.specificity)).arg(r.iterations);
    const QString note=QString("The model estimates P(Y=1). Outcome coding: Y=0 = \"%10\", Y=1 = \"%11\". Odds ratios are exp(coefficient); 95%% CIs are for the odds ratio. Classification uses a 0.50 probability cutoff. Confusion matrix: TP=%1, TN=%2, FP=%3, FN=%4. Complete-case accounting — total observations: %5; complete: %6; excluded blank: %7; declared missing: %8; invalid/non-numeric: %9.").arg(r.truePositive).arg(r.trueNegative).arg(r.falsePositive).arg(r.falseNegative).arg(r.observations).arg(r.complete).arg(r.excludedBlank).arg(r.excludedDeclaredMissing).arg(r.excludedNonNumeric).arg(r.outcomeLevel0).arg(r.outcomeLevel1);
    showResultTable("Binary Logistic Regression — "+y->currentText(),{"Term","Coefficient","Std. Error","z","p-value","Odds Ratio","95% OR low","95% OR high"},table,{1,2,3,4,5,6,7},note,summary);m_tabs->setCurrentWidget(m_output->parentWidget());statusBar()->showMessage("Logistic regression completed",5000);
}

void MainWindow::runPoissonRegression(){
    QStringList numeric;for(const auto& v:m_data.variables())if(v.type==VariableType::Numeric)numeric<<v.name;
    if(numeric.size()<2){QMessageBox::information(this,"Poisson Regression","A non-negative count outcome and at least one numeric predictor are required.");return;}
    QDialog d(this);d.setWindowTitle("Poisson Regression");d.resize(580,500);auto* l=new QVBoxLayout(&d);auto* f=new QFormLayout;auto* y=new QComboBox;y->addItems(numeric);f->addRow("Count outcome:",y);l->addLayout(f);l->addWidget(new QLabel("Numeric predictors:"));auto* p=new QListWidget;p->setSelectionMode(QAbstractItemView::ExtendedSelection);p->addItems(numeric);l->addWidget(p,1);
    auto* h=new QLabel("Poisson regression uses a log link. The outcome must be a non-negative integer count. Coefficients are reported with incidence-rate ratios (IRR). Predictors are numeric only.");h->setWordWrap(true);l->addWidget(h);auto* b=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);l->addWidget(b);connect(b,&QDialogButtonBox::accepted,&d,&QDialog::accept);connect(b,&QDialogButtonBox::rejected,&d,&QDialog::reject);if(d.exec()!=QDialog::Accepted)return;
    QStringList pn;for(auto* it:p->selectedItems())pn<<it->text();pn.removeAll(y->currentText());pn.removeDuplicates();if(pn.isEmpty()){QMessageBox::information(this,"Poisson Regression","Select at least one predictor different from the outcome.");return;}
    int yc=m_data.columnNames().indexOf(y->currentText());QVector<int> pc;for(const auto& n:pn)pc<<m_data.columnNames().indexOf(n);const auto r=AnalysisEngine::poissonRegression(m_data,yc,pc);if(r.complete<=r.parameters){showResultMessage("Poisson Regression",QString("Insufficient complete observations. %1 complete observations are available for a model with %2 parameters.").arg(r.complete).arg(r.parameters));return;}if(r.singular){showResultMessage("Poisson Regression","The model design is singular. Remove redundant or constant predictors.");return;}if(!r.converged){showResultMessage("Poisson Regression","The maximum-likelihood algorithm did not converge. Check for extreme counts, sparse data, or highly correlated predictors.");return;}
    QVector<QStringList> table;for(const auto& c:r.coefficients)table.push_back({c.term,AnalysisEngine::number(c.estimate),AnalysisEngine::number(c.stdError),AnalysisEngine::number(c.z),AnalysisEngine::number(c.p),AnalysisEngine::number(c.rateRatio),AnalysisEngine::number(c.ciLow),AnalysisEngine::number(c.ciHigh)});
    const QString summary=QString("Complete N: %1  |  Deviance: %2  |  Pearson χ²: %3  |  Pearson/df: %4  |  Log likelihood: %5  |  AIC: %6  |  BIC: %7  |  McFadden-like pseudo R²: %8").arg(r.complete).arg(AnalysisEngine::number(r.deviance)).arg(AnalysisEngine::number(r.pearsonChiSquare)).arg(AnalysisEngine::number(r.overdispersionRatio)).arg(AnalysisEngine::number(r.logLikelihood)).arg(AnalysisEngine::number(r.aic)).arg(AnalysisEngine::number(r.bic)).arg(AnalysisEngine::number(r.pseudoR2));
    const QString note=QString("Poisson GLM with log link. IRR = exp(coefficient), interpreted as the multiplicative change in expected count for a one-unit increase in the predictor, holding other predictors constant. Pearson/df substantially above 1 can indicate overdispersion. Complete-case accounting — total observations: %1; complete: %2; excluded blank: %3; declared missing: %4; invalid/non-numeric: %5.").arg(r.observations).arg(r.complete).arg(r.excludedBlank).arg(r.excludedDeclaredMissing).arg(r.excludedNonNumeric);
    showResultTable("Poisson Regression — "+y->currentText(),{"Term","Coefficient","Std. Error","z","p-value","IRR","95% IRR low","95% IRR high"},table,{1,2,3,4,5,6,7},note,summary);m_tabs->setCurrentWidget(m_output->parentWidget());statusBar()->showMessage("Poisson regression completed",5000);
}

void MainWindow::runNegativeBinomialRegression(){
    QStringList numeric;for(const auto& v:m_data.variables())if(v.type==VariableType::Numeric)numeric<<v.name;
    if(numeric.size()<2){QMessageBox::information(this,"Negative Binomial Regression","A non-negative count outcome and at least one numeric predictor are required.");return;}
    QDialog d(this);d.setWindowTitle("Negative Binomial Regression");d.resize(580,500);auto* l=new QVBoxLayout(&d);auto* f=new QFormLayout;auto* y=new QComboBox;y->addItems(numeric);f->addRow("Count outcome:",y);l->addLayout(f);l->addWidget(new QLabel("Numeric predictors:"));auto* p=new QListWidget;p->setSelectionMode(QAbstractItemView::ExtendedSelection);p->addItems(numeric);l->addWidget(p,1);
    auto* h=new QLabel("Negative Binomial regression uses a log link and estimates an NB2 dispersion parameter. The outcome must be a non-negative integer count. Coefficients are reported with incidence-rate ratios (IRR).");h->setWordWrap(true);l->addWidget(h);auto* b=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);l->addWidget(b);connect(b,&QDialogButtonBox::accepted,&d,&QDialog::accept);connect(b,&QDialogButtonBox::rejected,&d,&QDialog::reject);if(d.exec()!=QDialog::Accepted)return;
    QStringList pn;for(auto* it:p->selectedItems())pn<<it->text();pn.removeAll(y->currentText());pn.removeDuplicates();if(pn.isEmpty()){QMessageBox::information(this,"Negative Binomial Regression","Select at least one predictor different from the outcome.");return;}
    int yc=m_data.columnNames().indexOf(y->currentText());QVector<int> pc;for(const auto& n:pn)pc<<m_data.columnNames().indexOf(n);const auto r=AnalysisEngine::negativeBinomialRegression(m_data,yc,pc);if(r.complete<=r.parameters){showResultMessage("Negative Binomial Regression",QString("Insufficient complete observations. %1 complete observations are available for a model with %2 parameters.").arg(r.complete).arg(r.parameters));return;}if(r.singular){showResultMessage("Negative Binomial Regression","The model design is singular. Remove redundant or constant predictors.");return;}if(!r.converged){showResultMessage("Negative Binomial Regression","The maximum-likelihood algorithm did not converge. Check for extreme counts, sparse data, or highly correlated predictors.");return;}
    QVector<QStringList> table;for(const auto& c:r.coefficients)table.push_back({c.term,AnalysisEngine::number(c.estimate),AnalysisEngine::number(c.stdError),AnalysisEngine::number(c.z),AnalysisEngine::number(c.p),AnalysisEngine::number(c.rateRatio),AnalysisEngine::number(c.ciLow),AnalysisEngine::number(c.ciHigh)});
    const QString summary=QString("Complete N: %1  |  Deviance: %2  |  Pearson χ²: %3  |  Pearson/df: %4  |  Dispersion α: %5  |  Log likelihood: %6  |  AIC: %7  |  BIC: %8  |  McFadden-like pseudo R²: %9").arg(r.complete).arg(AnalysisEngine::number(r.deviance)).arg(AnalysisEngine::number(r.pearsonChiSquare)).arg(AnalysisEngine::number(r.overdispersionRatio)).arg(AnalysisEngine::number(r.dispersion)).arg(AnalysisEngine::number(r.logLikelihood)).arg(AnalysisEngine::number(r.aic)).arg(AnalysisEngine::number(r.bic)).arg(AnalysisEngine::number(r.pseudoR2));
    const QString note=QString("Negative Binomial GLM (NB2) with log link. IRR = exp(coefficient). The variance is modeled as μ + αμ², allowing overdispersion relative to Poisson. Complete-case accounting — total observations: %1; complete: %2; excluded blank: %3; declared missing: %4; invalid/non-numeric: %5.").arg(r.observations).arg(r.complete).arg(r.excludedBlank).arg(r.excludedDeclaredMissing).arg(r.excludedNonNumeric);
    showResultTable("Negative Binomial Regression — "+y->currentText(),{"Term","Coefficient","Std. Error","z","p-value","IRR","95% IRR low","95% IRR high"},table,{1,2,3,4,5,6,7},note,summary);m_tabs->setCurrentWidget(m_output->parentWidget());statusBar()->showMessage("Negative binomial regression completed",5000);
}

void MainWindow::runTimeSeriesAnalysis(){
    QStringList all=m_data.columnNames(), numeric;for(const auto& v:m_data.variables())if(v.type==VariableType::Numeric)numeric<<v.name;if(numeric.isEmpty()||all.size()<2){QMessageBox::information(this,"Time Series Analysis","A numeric series is required.");return;}
    QDialog d(this);d.setWindowTitle("Time Series Analysis");d.resize(560,400);auto* f=new QFormLayout(&d);auto* t=new QComboBox;t->addItems(all);auto* y=new QComboBox;y->addItems(numeric);auto* w=new QSpinBox;w->setRange(1,50);w->setValue(3);f->addRow("Time / order variable:",t);f->addRow("Series variable:",y);f->addRow("Moving-average window:",w);auto* h=new QLabel("Rows are analyzed in their current dataset order. The output includes lag-1, first difference, percent change, moving average, trend slope/R² and lag-1 autocorrelation.");h->setWordWrap(true);f->addRow(h);auto* b=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);f->addRow(b);connect(b,&QDialogButtonBox::accepted,&d,&QDialog::accept);connect(b,&QDialogButtonBox::rejected,&d,&QDialog::reject);if(d.exec()!=QDialog::Accepted)return;int tc=m_data.columnNames().indexOf(t->currentText()),yc=m_data.columnNames().indexOf(y->currentText());const auto r=AnalysisEngine::timeSeriesAnalysis(m_data,tc,yc,w->value());if(r.valid<2){showResultMessage("Time Series Analysis","At least 2 valid numeric observations are required.");return;}
    QVector<QStringList> table;for(const auto& z:r.rows)table.push_back({QString::number(z.observation),z.time,AnalysisEngine::number(z.value),AnalysisEngine::number(z.lag1),AnalysisEngine::number(z.difference),AnalysisEngine::number(z.percentChange),AnalysisEngine::number(z.movingAverage)});
    const QString summary=QString("Valid N: %1  |  Mean: %2  |  SD: %3  |  Min: %4  |  Max: %5  |  Trend slope: %6  |  Trend R²: %7  |  Lag-1 ACF: %8").arg(r.valid).arg(AnalysisEngine::number(r.mean)).arg(AnalysisEngine::number(r.stdDev)).arg(AnalysisEngine::number(r.min)).arg(AnalysisEngine::number(r.max)).arg(AnalysisEngine::number(r.trendSlope)).arg(AnalysisEngine::number(r.trendR2)).arg(AnalysisEngine::number(r.acf1));
    const QString note=QString("Time-series calculations use the current row order after filtering. First differences compare each valid series observation with the immediately preceding valid series observation. Moving average window = %1. Excluded series observations — blank: %2; declared missing: %3; invalid/non-numeric: %4. This module does not automatically reorder observations by the time field.").arg(r.movingWindow).arg(r.excludedBlank).arg(r.excludedDeclaredMissing).arg(r.excludedNonNumeric);
    showResultTable("Time Series Analysis — "+y->currentText(),{"Observation","Time","Value","Lag 1","Difference","% Change","Moving Average"},table,{2,3,4,5,6},note,summary);m_tabs->setCurrentWidget(m_output->parentWidget());statusBar()->showMessage("Time series analysis completed",5000);
}

void MainWindow::runEconometricRobustOls(){
    QStringList numeric;for(const auto& v:m_data.variables())if(v.type==VariableType::Numeric)numeric<<v.name;if(numeric.size()<2){QMessageBox::information(this,"Econometrics","At least two numeric variables are required.");return;}
    QDialog d(this);d.setWindowTitle("Econometric OLS — Robust Standard Errors");d.resize(560,500);auto* l=new QVBoxLayout(&d);auto* f=new QFormLayout;auto* y=new QComboBox;y->addItems(numeric);f->addRow("Outcome variable:",y);l->addLayout(f);l->addWidget(new QLabel("Numeric predictors:"));auto* p=new QListWidget;p->setSelectionMode(QAbstractItemView::ExtendedSelection);p->addItems(numeric);l->addWidget(p,1);auto* b=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);l->addWidget(b);connect(b,&QDialogButtonBox::accepted,&d,&QDialog::accept);connect(b,&QDialogButtonBox::rejected,&d,&QDialog::reject);if(d.exec()!=QDialog::Accepted)return;QStringList pn;for(auto* it:p->selectedItems())pn<<it->text();pn.removeAll(y->currentText());pn.removeDuplicates();if(pn.isEmpty()){QMessageBox::information(this,"Econometrics","Select at least one predictor.");return;}int yc=m_data.columnNames().indexOf(y->currentText());QVector<int> pc;for(const auto& n:pn)pc<<m_data.columnNames().indexOf(n);const auto r=AnalysisEngine::econometricRobustOls(m_data,yc,pc);if(r.complete<=r.parameters){showResultMessage("Econometrics",QString("Insufficient complete observations: %1 complete observations for %2 parameters.").arg(r.complete).arg(r.parameters));return;}if(r.singular){showResultMessage("Econometrics","The design matrix is singular. Remove redundant or constant predictors.");return;}
    QVector<QStringList> table;for(const auto& c:r.coefficients)table.push_back({c.term,AnalysisEngine::number(c.estimate),AnalysisEngine::number(c.robustStdError),AnalysisEngine::number(c.zOrT),AnalysisEngine::number(c.pValue),AnalysisEngine::number(c.ciLow),AnalysisEngine::number(c.ciHigh)});
    const QString summary=QString("Complete N: %1  |  R²: %2  |  Adjusted R²: %3  |  RMSE: %4  |  F(%5,%6): %7  |  Model p: %8  |  Durbin–Watson: %9  |  Breusch–Pagan LM: %10  |  BP p: %11").arg(r.complete).arg(AnalysisEngine::number(r.rSquared)).arg(AnalysisEngine::number(r.adjustedRSquared)).arg(AnalysisEngine::number(r.rmse)).arg(r.predictors).arg(r.complete-r.parameters).arg(AnalysisEngine::number(r.f)).arg(AnalysisEngine::number(r.fP)).arg(AnalysisEngine::number(r.durbinWatson)).arg(AnalysisEngine::number(r.breuschPaganLM)).arg(AnalysisEngine::number(r.breuschPaganP));
    const QString note=QString("Coefficient inference uses HC1 heteroskedasticity-consistent standard errors. The Breusch–Pagan test uses n×R² from an auxiliary regression of squared residuals on the predictors; a small p-value indicates evidence of heteroskedasticity. Complete-case accounting — total observations: %1; complete: %2; excluded blank: %3; declared missing: %4; invalid/non-numeric: %5.").arg(r.observations).arg(r.complete).arg(r.excludedBlank).arg(r.excludedDeclaredMissing).arg(r.excludedNonNumeric);
    showResultTable("Econometric Robust OLS — "+y->currentText(),{"Term","Estimate","HC1 Std. Error","z/t","p-value","95% CI low","95% CI high"},table,{1,2,3,4,5,6},note,summary);m_tabs->setCurrentWidget(m_output->parentWidget());statusBar()->showMessage("Econometric robust OLS completed",5000);
}

void MainWindow::runADFTest(){
    QStringList numeric;for(const auto& v:m_data.variables())if(v.type==VariableType::Numeric)numeric<<v.name;if(numeric.isEmpty()){QMessageBox::information(this,"ADF Unit Root Test","At least one numeric series is required.");return;}bool ok=false;QString name=QInputDialog::getItem(this,"Augmented Dickey–Fuller Unit Root Test","Series:",numeric,0,false,&ok);if(!ok)return;int c=m_data.columnNames().indexOf(name);const auto r=AnalysisEngine::augmentedDickeyFuller(m_data,c);if(r.valid<6){showResultMessage("ADF Unit Root Test","At least 6 valid numeric observations are required for this no-augmentation ADF implementation.");return;}QVector<QStringList> table={{"ADF statistic",AnalysisEngine::number(r.statistic)},{"1% critical value",AnalysisEngine::number(r.critical1)},{"5% critical value",AnalysisEngine::number(r.critical5)},{"10% critical value",AnalysisEngine::number(r.critical10)},{"Valid observations",QString::number(r.valid)},{"Conclusion",r.conclusion}};showResultTable("Augmented Dickey–Fuller Test — "+name,{"Statistic","Value"},table,{1},"This implementation estimates ΔYt = α + γY(t−1) + εt, i.e. an ADF/DF test with zero augmentation lags. Critical values shown are standard approximate values for the intercept-only case; use them as diagnostics rather than as a substitute for a full lag-selection procedure.",QString("Valid N: %1  |  Test statistic: %2").arg(r.valid).arg(AnalysisEngine::number(r.statistic)));m_tabs->setCurrentWidget(m_output->parentWidget());statusBar()->showMessage("ADF test completed",5000);
}

void MainWindow::formatResultsTables(){
    QSettings settings("StatPro Analytics","StatPro Analytics");
    QDialog d(this); d.setWindowTitle("Results Table Formatting"); d.resize(460,360);
    auto* form=new QFormLayout(&d);
    auto* shade=new QComboBox; shade->addItems({"None","Alternating rows"}); shade->setCurrentText(settings.value("results/rowShading","None").toString());
    auto* grid=new QComboBox; grid->addItems({"Visible","Hidden"}); grid->setCurrentText(settings.value("results/grid","Visible").toString());
    auto* decimals=new QSpinBox; decimals->setRange(0,8); decimals->setValue(settings.value("results/decimals",4).toInt());
    auto* fontSize=new QSpinBox; fontSize->setRange(8,20); fontSize->setValue(settings.value("results/fontSize",10).toInt());
    auto* headerBg=new QPushButton("Choose…"); auto* rowBg=new QPushButton("Choose…"); auto* altBg=new QPushButton("Choose…");
    QColor h=settings.value("results/headerColor", "#eef1f4").value<QColor>();
    QColor r=settings.value("results/rowColor", "#ffffff").value<QColor>();
    QColor a=settings.value("results/alternateColor", "#f7f9fb").value<QColor>();
    if(!h.isValid())h=QColor("#eef1f4"); if(!r.isValid())r=QColor("#ffffff"); if(!a.isValid())a=QColor("#f7f9fb");
    auto choose=[&](QPushButton* b,QColor& color){b->setText(color.name()); connect(b,&QPushButton::clicked,&d,[&color,b,&d]{const QColor picked=QColorDialog::getColor(color,&d,"Choose table color");if(picked.isValid()){color=picked;b->setText(color.name());}});};
    choose(headerBg,h); choose(rowBg,r); choose(altBg,a);
    form->addRow("Row shading",shade); form->addRow("Grid lines",grid); form->addRow("Decimal places",decimals); form->addRow("Font size",fontSize);
    form->addRow("Header background",headerBg); form->addRow("Row background",rowBg); form->addRow("Alternate-row background",altBg);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel); form->addRow(buttons);
    connect(buttons,&QDialogButtonBox::accepted,&d,&QDialog::accept); connect(buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);
    if(d.exec()!=QDialog::Accepted)return;
    settings.setValue("results/rowShading",shade->currentText()); settings.setValue("results/grid",grid->currentText()); settings.setValue("results/decimals",decimals->value()); settings.setValue("results/fontSize",fontSize->value());
    settings.setValue("results/headerColor",h); settings.setValue("results/rowColor",r); settings.setValue("results/alternateColor",a);
    applyResultsFormatting();
}

void MainWindow::applyResultsFormatting(){
    if(!m_output)return;
    QSettings settings("StatPro Analytics","StatPro Analytics");
    const QString shade=settings.value("results/rowShading","None").toString();
    const QString grid=settings.value("results/grid","Visible").toString();
    const int fontSize=settings.value("results/fontSize",10).toInt();
    const QPalette pal=m_output->palette();
    const QColor defaultBase=pal.color(QPalette::Base);
    const QColor defaultAlt=pal.color(QPalette::AlternateBase);
    const QColor defaultHeader=pal.color(QPalette::Mid);
    const QColor h=settings.contains("results/headerColor")?settings.value("results/headerColor").value<QColor>():defaultHeader;
    const QColor r=settings.contains("results/rowColor")?settings.value("results/rowColor").value<QColor>():defaultBase;
    const QColor a=settings.contains("results/alternateColor")?settings.value("results/alternateColor").value<QColor>():defaultAlt;
    const QColor gridColor=grid=="Hidden"?r:pal.color(QPalette::Mid);
    const int decimals=settings.value("results/decimals",4).toInt();
    for(int row=0;row<m_output->rowCount();++row){
        for(int col=0;col<m_output->columnCount();++col){
            auto* item=m_output->item(row,col); if(!item || !item->data(Qt::UserRole).isValid()) continue;
            const double value=item->data(Qt::UserRole).toDouble();
            const bool percent=item->data(Qt::UserRole+1).toBool();
            const bool whole=std::fabs(value-std::round(value))<1e-9 && !percent;
            item->setText(whole?QString::number(value,'f',0):QString::number(value,'f',decimals)+(percent?"%":""));
        }
    }
    QString style=QString("QTableWidget{font-size:%1pt;background:%2;color:palette(text);gridline-color:%3;} QTableWidget::item{background:%2;padding:4px 6px;} QTableWidget::item:selected{background:palette(highlight);color:palette(highlighted-text);} QHeaderView::section{background:%4;color:palette(text);font-weight:600;padding:6px;border:0;border-bottom:1px solid %3;}")
        .arg(fontSize).arg(r.name(QColor::HexArgb)).arg(gridColor.name(QColor::HexArgb)).arg(h.name(QColor::HexArgb));
    if(shade=="Alternating rows") style += QString(" QTableWidget::item:alternate{background:%1;}").arg(a.name(QColor::HexArgb));
    m_output->setStyleSheet(style);
}

void MainWindow::updateTitle(){
    setWindowTitle(QString("StatPro Analytics %1 — %2%3").arg(QApplication::applicationVersion(),m_state.projectName(),m_state.dirty()?" *":""));
}

void MainWindow::updateStatus(){
    if(!m_statusRows || !m_statusVariables || !m_statusSelection || !m_statusState || !m_statusMode)return;

    m_statusRows->setText(QString("%1 observations").arg(m_data.rowCount()));
    m_statusVariables->setText(QString("%1 variables").arg(m_data.columnCount()));

    int selected=0;
    if(m_grid)selected=m_grid->selectedItems().size();
    m_statusSelection->setText(selected>0 ? QString("%1 cell%2 selected").arg(selected).arg(selected==1?"":"s") : "No selection");

    m_statusMode->setText("Offline");
    m_statusState->setText(m_state.dirty() ? "Modified" : "Ready");
}
bool MainWindow::confirmSaveIfDirty(){if(!m_state.dirty())return true;const auto ans=QMessageBox::warning(this,"Unsaved Changes","This project has unsaved changes.",QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel);if(ans==QMessageBox::Save){saveProject();return !m_state.dirty();}return ans==QMessageBox::Discard;}
void MainWindow::closeEvent(QCloseEvent* e){if(confirmSaveIfDirty())e->accept();else e->ignore();}
void MainWindow::applyTheme(){
 if(m_state.darkMode())setStyleSheet(R"(QMainWindow,QDockWidget,QTabWidget::pane{background:#20242a;color:#e8eaed}QToolBar{background:#292e36;border:1px solid #3b414b;padding:5px;spacing:4px}QToolButton{color:#e8eaed;padding:7px 9px;border-radius:5px}QToolButton:hover{background:#3a414c}QTableWidget,QListWidget,QTreeWidget,QPlainTextEdit,QLineEdit,QComboBox{background:#252a31;color:#e8eaed;border:1px solid #414752}QHeaderView::section{background:#303640;color:#e8eaed;padding:5px}QPushButton{color:#e8eaed;background:#303640;border:1px solid #4b5360;padding:6px 12px;border-radius:4px}QMenuBar,QMenu{background:#292e36;color:#e8eaed}QMenuBar::item:selected,QMenu::item:selected{background:#3a414c}QStatusBar{background:#292e36;color:#e8eaed;border-top:1px solid #3b414b}QDockWidget::title{background:#292e36;color:#e8eaed;padding:6px}QGroupBox{border:1px solid #414752;margin-top:8px;padding-top:8px;font-weight:600}QPushButton:hover{background:#3a414c})");
 else setStyleSheet(R"(QMainWindow,QDockWidget,QTabWidget::pane{background:#f4f6f8;color:#20242a}QToolBar{background:#ffffff;border:1px solid #d8dde3;padding:5px;spacing:4px}QToolButton{color:#20242a;padding:7px 9px;border-radius:5px}QToolButton:hover{background:#e9eef5}QTableWidget,QListWidget,QTreeWidget,QPlainTextEdit,QLineEdit,QComboBox{background:#ffffff;color:#20242a;border:1px solid #d8dde3}QHeaderView::section{background:#eef1f4;color:#20242a;padding:5px}QPushButton{color:#20242a;background:#ffffff;border:1px solid #b8c0c9;padding:6px 12px;border-radius:4px}QMenuBar,QMenu{background:#ffffff;color:#20242a}QMenuBar::item:selected,QMenu::item:selected{background:#e9eef5}QStatusBar{background:#ffffff;color:#20242a;border-top:1px solid #d8dde3}QDockWidget::title{background:#eef1f4;color:#20242a;padding:6px}QGroupBox{border:1px solid #d8dde3;margin-top:8px;padding-top:8px;font-weight:600}QPushButton:hover{background:#e9eef5})");
 applyResultsFormatting();
}
}
