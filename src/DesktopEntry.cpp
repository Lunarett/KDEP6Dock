#include "DesktopEntry.h"

#include <QFile>
#include <QTextStream>

bool DesktopEntry::parseFile(const QString &path, DockAppEntry &entry, QString &error) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = QStringLiteral("Unable to open file");
        return false;
    }

    bool inDesktopSection = false;
    QString name;
    QString icon;
    QString exec;

    QTextStream stream(&file);
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        if (line.startsWith('[') && line.endsWith(']')) {
            inDesktopSection = (line == "[Desktop Entry]");
            continue;
        }

        if (!inDesktopSection) {
            continue;
        }

        const int splitIndex = line.indexOf('=');
        if (splitIndex <= 0) {
            continue;
        }

        const QString key = line.left(splitIndex).trimmed();
        const QString value = line.mid(splitIndex + 1).trimmed();

        if (key == "Name") {
            name = value;
        } else if (key == "Icon") {
            icon = value;
        } else if (key == "Exec") {
            exec = sanitizeExec(value);
        }
    }

    if (name.isEmpty() || exec.isEmpty()) {
        error = QStringLiteral("Missing required Name or Exec field");
        return false;
    }

    entry.desktopFile = path;
    entry.name = name;
    entry.icon = icon;
    entry.exec = exec;
    return true;
}

QString DesktopEntry::sanitizeExec(QString exec) {
    static const QStringList placeholders = {
        "%f", "%F", "%u", "%U", "%d", "%D", "%n", "%N", "%i", "%c", "%k", "%v", "%m"
    };

    for (const QString &token : placeholders) {
        exec.replace(token, "");
    }

    return exec.simplified();
}
