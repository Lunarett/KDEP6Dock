#include "Config.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QStandardPaths>

Config::Config() {
    const QString baseDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    const QString dirPath = baseDir + "/kdep6dock";
    m_configPath = dirPath + "/config.json";
}

bool Config::load() {
    QFile file(m_configPath);
    if (!file.exists()) {
        qWarning() << "Config file not found, using defaults:" << m_configPath;
        return false;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open config file:" << m_configPath;
        return false;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        qWarning() << "Invalid JSON config, using defaults:" << m_configPath;
        return false;
    }

    const QJsonObject obj = doc.object();
    m_data.baseIconSize = obj.value("baseIconSize").toInt(m_data.baseIconSize);
    m_data.maxScale = obj.value("maxScale").toDouble(m_data.maxScale);
    m_data.spacing = obj.value("spacing").toInt(m_data.spacing);
    m_data.animationDurationMs = obj.value("animationDurationMs").toInt(m_data.animationDurationMs);
    m_data.neighborRadius = obj.value("neighborRadius").toInt(m_data.neighborRadius);
    m_data.dockPadding = obj.value("dockPadding").toInt(m_data.dockPadding);
    m_data.dockMarginBottom = obj.value("dockMarginBottom").toInt(m_data.dockMarginBottom);
    m_data.backgroundOpacity = obj.value("backgroundOpacity").toDouble(m_data.backgroundOpacity);
    m_data.position = obj.value("position").toString(m_data.position);

    m_data.pinnedApps.clear();
    for (const QJsonValue &value : obj.value("pinnedApps").toArray()) {
        if (!value.isObject()) {
            continue;
        }
        m_data.pinnedApps.push_back(appFromJson(value.toObject()));
    }

    qDebug() << "Loaded config:" << m_configPath << "apps:" << m_data.pinnedApps.size();
    return true;
}

bool Config::save() const {
    QJsonObject obj;
    obj.insert("baseIconSize", m_data.baseIconSize);
    obj.insert("maxScale", m_data.maxScale);
    obj.insert("spacing", m_data.spacing);
    obj.insert("animationDurationMs", m_data.animationDurationMs);
    obj.insert("neighborRadius", m_data.neighborRadius);
    obj.insert("dockPadding", m_data.dockPadding);
    obj.insert("dockMarginBottom", m_data.dockMarginBottom);
    obj.insert("backgroundOpacity", m_data.backgroundOpacity);
    obj.insert("position", m_data.position);

    QJsonArray apps;
    for (const DockAppEntry &app : m_data.pinnedApps) {
        apps.push_back(appToJson(app));
    }
    obj.insert("pinnedApps", apps);

    QDir dir;
    const QFileInfo info(m_configPath);
    if (!dir.mkpath(info.absolutePath())) {
        qWarning() << "Failed to create config directory:" << info.absolutePath();
        return false;
    }

    QFile file(m_configPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "Failed to save config file:" << m_configPath;
        return false;
    }

    const QJsonDocument doc(obj);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    qDebug() << "Saved config:" << m_configPath;
    return true;
}

const DockConfig &Config::data() const {
    return m_data;
}

DockConfig &Config::data() {
    return m_data;
}

QString Config::configPath() const {
    return m_configPath;
}

QJsonObject Config::appToJson(const DockAppEntry &entry) const {
    QJsonObject obj;
    obj.insert("desktopFile", entry.desktopFile);
    obj.insert("name", entry.name);
    obj.insert("icon", entry.icon);
    obj.insert("exec", entry.exec);
    return obj;
}

DockAppEntry Config::appFromJson(const QJsonObject &obj) const {
    DockAppEntry entry;
    entry.desktopFile = obj.value("desktopFile").toString();
    entry.name = obj.value("name").toString();
    entry.icon = obj.value("icon").toString();
    entry.exec = obj.value("exec").toString();
    return entry;
}
