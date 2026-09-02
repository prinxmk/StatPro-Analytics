#include "AppState.h"
#include <QSettings>
namespace StatPro {
AppState::AppState(QObject* parent) : QObject(parent) {
    QSettings s;
    m_darkMode = s.value("theme/dark", false).toBool();
    m_recentProjects = s.value("projects/recent").toStringList();
}
QString AppState::projectName() const { return m_projectName; }
void AppState::setProjectName(const QString& v) {
    if (m_projectName == v) return;
    m_projectName = v; emit projectNameChanged(v);
}
QString AppState::projectPath() const { return m_projectPath; }
void AppState::setProjectPath(const QString& v) {
    if (m_projectPath == v) return;
    m_projectPath = v; emit projectPathChanged(v);
}
bool AppState::darkMode() const { return m_darkMode; }
void AppState::setDarkMode(bool v) {
    if (m_darkMode == v) return;
    m_darkMode = v; QSettings().setValue("theme/dark", v); emit themeChanged(v);
}
bool AppState::dirty() const { return m_dirty; }
void AppState::setDirty(bool v) {
    if (m_dirty == v) return;
    m_dirty = v; emit dirtyChanged(v);
}
QStringList AppState::recentProjects() const { return m_recentProjects; }
void AppState::addRecentProject(const QString& path) {
    m_recentProjects.removeAll(path);
    m_recentProjects.prepend(path);
    while (m_recentProjects.size() > 10) m_recentProjects.removeLast();
    QSettings().setValue("projects/recent", m_recentProjects);
    emit recentProjectsChanged();
}
}
