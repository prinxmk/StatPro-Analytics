#pragma once
#include <QObject>
#include <QString>
#include <QStringList>

namespace StatPro {
class AppState : public QObject {
    Q_OBJECT
public:
    explicit AppState(QObject* parent = nullptr);
    QString projectName() const;
    void setProjectName(const QString&);
    QString projectPath() const;
    void setProjectPath(const QString&);
    bool darkMode() const;
    void setDarkMode(bool);
    bool dirty() const;
    void setDirty(bool);
    QStringList recentProjects() const;
    void addRecentProject(const QString&);
signals:
    void projectNameChanged(const QString&);
    void projectPathChanged(const QString&);
    void themeChanged(bool);
    void dirtyChanged(bool);
    void recentProjectsChanged();
private:
    QString m_projectName{"Untitled Project"};
    QString m_projectPath;
    bool m_darkMode{false};
    bool m_dirty{false};
    QStringList m_recentProjects;
};
}
