#pragma once

#include "DockModel.h"

#include <QString>

class DesktopEntry {
public:
    static bool parseFile(const QString &path, DockAppEntry &entry, QString &error);

private:
    static QString sanitizeExec(QString exec);
};
