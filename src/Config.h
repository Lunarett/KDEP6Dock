#pragma once

#include "DockModel.h"

#include <QJsonObject>
#include <QString>

struct DockConfig {
    int baseIconSize = 48;
    double maxScale = 1.5;
    int spacing = 10;
    int animationDurationMs = 180;
    int neighborRadius = 2;
    int dockPadding = 12;
    int dockMarginBottom = 24;
    double backgroundOpacity = 0.65;
    QString position = "bottom";
    QVector<DockAppEntry> pinnedApps;
};

class Config {
public:
    Config();

    bool load();
    bool save() const;

    const DockConfig &data() const;
    DockConfig &data();

    QString configPath() const;

private:
    QJsonObject appToJson(const DockAppEntry &entry) const;
    DockAppEntry appFromJson(const QJsonObject &obj) const;

    DockConfig m_data;
    QString m_configPath;
};
