#include "DockModel.h"

DockModel::DockModel(QObject *parent)
    : QObject(parent) {
}

const QVector<DockAppEntry> &DockModel::apps() const {
    return m_apps;
}

int DockModel::count() const {
    return m_apps.size();
}

void DockModel::setApps(const QVector<DockAppEntry> &apps) {
    m_apps = apps;
    emit modelChanged();
}

void DockModel::addApp(const DockAppEntry &app) {
    m_apps.push_back(app);
    emit modelChanged();
}

bool DockModel::removeAt(int index) {
    if (index < 0 || index >= m_apps.size()) {
        return false;
    }
    m_apps.removeAt(index);
    emit modelChanged();
    return true;
}

bool DockModel::move(int from, int to) {
    if (from < 0 || from >= m_apps.size() || to < 0 || to >= m_apps.size() || from == to) {
        return false;
    }

    m_apps.move(from, to);
    emit modelChanged();
    return true;
}
