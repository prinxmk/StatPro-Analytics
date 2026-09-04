#pragma once
#include <QMainWindow>
#include <QPointer>
#include <QUndoStack>
#include "../core/AppState.h"
#include "../data/DataSet.h"

class QTableWidget;
class QListWidget;
class QTreeWidget;
class QLabel;
class QTabWidget;
class QPlainTextEdit;
class QLineEdit;
class QPushButton;
class QComboBox;
class QDockWidget;

namespace StatPro {

class MainWindow : public QMainWindow {
    Q_OBJECT
    friend class CellEditCommand;

public:
    explicit MainWindow(QWidget* parent=nullptr);

protected:
    void closeEvent(QCloseEvent*) override;

private slots:
    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();
    void importCsv();
    void exportCsv();
    void toggleTheme();

    void addVariable();
    void editVariable();
    void deleteVariable();
    void applyFilter();
    void clearFilter();
    void replaceText();

    void cellChanged(int row,int column);
    void copySelection();
    void pasteSelection();
    void addRow();
    void insertRow();
    void deleteRows();
    void sortAscending();
    void sortDescending();
    void showDatasetInfo();
    void runDescriptiveStatistics();
    void runFrequencies();
    void runSummaryByGroup();
    void undo();
    void redo();

private:
    void buildInterface();
    void buildMenus();
    void buildVariablesPanel();
    void buildPropertiesPanel();
    void buildDataCommandPanel();
    void buildStatusBar();

    void refreshDataView();
    void refreshDataView(const QVector<int>& rows);
    void refreshVariables();
    void refreshProperties(int variableIndex=-1);
    void updateTitle();
    void applyTheme();
    void updateStatus();

    int selectedVariableColumn() const;
    void selectVariableColumn(int column);

    bool confirmSaveIfDirty();
    bool setCellFromText(int dataRow,int column,const QString& text,QString* error=nullptr);
    QString displayValue(int row,int column) const;
    QVector<int> visibleRows() const;
    int dataRowForViewRow(int viewRow) const;

    AppState m_state;
    DataSet m_data;

    QTabWidget* m_tabs{};
    QTableWidget* m_grid{};
    QListWidget* m_variables{};
    QTreeWidget* m_properties{};
    QPlainTextEdit* m_output{};
    QLineEdit* m_filterEdit{};
    QUndoStack* m_undoStack{};

    QDockWidget* m_variablesDock{};
    QDockWidget* m_propertiesDock{};
    QDockWidget* m_commandsDock{};

    QLabel* m_statusState{};
    QLabel* m_statusRows{};
    QLabel* m_statusVariables{};
    QLabel* m_statusSelection{};
    QLabel* m_statusMode{};

    bool m_refreshing=false;
    QVector<int> m_visibleRows;
};

}
