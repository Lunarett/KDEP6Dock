#pragma once

#include <QObject>
#include <QString>
#include <QVector>

struct DockAppEntry {
    QString desktopFile;
    QString name;
    QString icon;
    QString exec;
};

class DockModel : public QObject {
    Q_OBJECT
public:
    explicit DockModel(QObject *parent = nullptr);

    const QVector<DockAppEntry> &apps() const;
    int count() const;

    void setApps(const QVector<DockAppEntry> &apps);
    void addApp(const DockAppEntry &app);
    bool removeAt(int index);
    bool move(int from, int to);

signals:
    void modelChanged();

private:
    QVector<DockAppEntry> m_apps;
};
