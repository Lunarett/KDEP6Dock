#include "DockWindow.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("kdep6dock");
    app.setOrganizationName("kdep6dock");

    DockWindow window;
    window.show();

    return app.exec();
}
