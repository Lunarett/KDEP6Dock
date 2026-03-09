#include "DockWindow.h"

#include "DesktopEntry.h"

#include <QApplication>
#include <QDragEnterEvent>
#include <QEvent>
#include <QGuiApplication>
#include <QLayout>
#include <QMouseEvent>
#include <QMimeData>
#include <QPainter>
#include <QProcess>
#include <QScreen>
#include <QTimer>
#include <QUrl>
#include <algorithm>
#include <cmath>

#if defined(Q_OS_LINUX) && __has_include(<X11/Xatom.h>)
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#define KDEP6DOCK_HAS_X11 1
#else
#define KDEP6DOCK_HAS_X11 0
#endif

DockWindow::DockWindow(QWidget *parent)
    : QWidget(parent) {
    setAcceptDrops(true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_X11DoNotAcceptFocus, true);
    setAttribute(Qt::WA_X11NetWmWindowTypeDock, true);
    setMouseTracking(true);

    setWindowFlags(Qt::FramelessWindowHint |
                   Qt::Tool |
                   Qt::WindowStaysOnTopHint |
                   Qt::WindowDoesNotAcceptFocus);

    const QString platform = QGuiApplication::platformName().toLower();
    m_isWayland = platform.contains("wayland");
    m_isX11 = platform.contains("xcb") || platform.contains("x11");
    qDebug() << "Platform:" << platform
             << "(x11=" << m_isX11 << ", wayland=" << m_isWayland << ")";

    m_config.load();
    m_model.setApps(m_config.data().pinnedApps);

    auto *outerLayout = new QHBoxLayout(this);
    outerLayout->setContentsMargins(m_config.data().dockPadding, m_config.data().dockPadding,
                                    m_config.data().dockPadding, m_config.data().dockPadding);

    m_container = new QWidget(this);
    m_container->setMouseTracking(true);
    m_container->installEventFilter(this);

    m_layout = new QHBoxLayout(m_container);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(m_config.data().spacing);
    outerLayout->addWidget(m_container);

    connect(&m_model, &DockModel::modelChanged, this, &DockWindow::requestRebuildUi);
    connect(&m_animationTimer, &QTimer::timeout, this, &DockWindow::tickAnimations);
    connect(&m_overlapCheckTimer, &QTimer::timeout, this, &DockWindow::updateDodgeOverlapState);

    m_animationTimer.start(16);
    m_overlapCheckTimer.start(150);
    m_elapsed.start();

    rebuildUi();
}

DockWindow::~DockWindow() {
    syncModelToConfig();
    m_config.save();
}

void DockWindow::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QColor bg(28, 28, 30, static_cast<int>(255 * m_config.data().backgroundOpacity));
    painter.setBrush(bg);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), 18, 18);
}

void DockWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasFormat("application/x-kdep6dock-index") || event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void DockWindow::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData()->hasFormat("application/x-kdep6dock-index") || event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void DockWindow::dropEvent(QDropEvent *event) {
    if (event->mimeData()->hasFormat("application/x-kdep6dock-index")) {
        const int from = event->mimeData()->data("application/x-kdep6dock-index").toInt();
        int to = insertionIndexForPos(event->position().toPoint());
        to = qBound(0, to, m_model.count());
        if (from < to) {
            --to;
        }

        qDebug() << "Internal reorder drag:" << from << "->" << to;
        if (m_model.move(from, to)) {
            syncModelToConfig();
            m_config.save();
        }
        event->acceptProposedAction();
        return;
    }

    if (event->mimeData()->hasUrls()) {
        bool changed = false;
        for (const QUrl &url : event->mimeData()->urls()) {
            if (!url.isLocalFile()) {
                continue;
            }
            const QString path = url.toLocalFile();
            if (!path.endsWith(".desktop")) {
                qWarning() << "Ignored non-desktop drop:" << path;
                continue;
            }
            changed |= addDesktopFile(path);
        }

        if (changed) {
            syncModelToConfig();
            m_config.save();
        }

        event->acceptProposedAction();
    }
}

bool DockWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_container) {
        if (event->type() == QEvent::MouseMove) {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            m_cursorXInContainer = mouseEvent->position().x();
            updateTargets();
        } else if (event->type() == QEvent::Leave) {
            clearCursorInfluence();
        }
    }

    return QWidget::eventFilter(watched, event);
}

void DockWindow::leaveEvent(QEvent *event) {
    Q_UNUSED(event)
    clearCursorInfluence();
}

void DockWindow::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    positionDock();
    applyOverlapPolicy();
}

void DockWindow::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    QTimer::singleShot(0, this, &DockWindow::positionDock);
    QTimer::singleShot(0, this, &DockWindow::applyShellWindowHints);
    QTimer::singleShot(0, this, &DockWindow::applyOverlapPolicy);
}

void DockWindow::requestRebuildUi() {
    if (m_rebuildQueued) {
        return;
    }

    m_rebuildQueued = true;
    QTimer::singleShot(0, this, &DockWindow::rebuildUi);
}

void DockWindow::rebuildUi() {
    m_rebuildQueued = false;

    qDeleteAll(m_itemWidgets);
    m_itemWidgets.clear();

    while (QLayoutItem *item = m_layout->takeAt(0)) {
        delete item;
    }

    const int slotSize = qRound(m_config.data().baseIconSize * m_config.data().maxScale);

    int index = 0;
    for (const DockAppEntry &app : m_model.apps()) {
        auto *item = new DockItemWidget(app, index, m_container);
        item->setBaseIconSize(m_config.data().baseIconSize);
        item->setMaxScale(m_config.data().maxScale);
        item->setFixedSize(slotSize + 8, slotSize + 8);

        connect(item, &DockItemWidget::pointerMovedGlobal, this, &DockWindow::onItemPointerMoved);
        connect(item, &DockItemWidget::clicked, this, &DockWindow::onItemClicked);
        connect(item, &DockItemWidget::removeRequested, this, &DockWindow::onItemRemoveRequested);

        m_layout->addWidget(item);
        m_itemWidgets.push_back(item);
        ++index;
    }

    if (layout()) {
        layout()->activate();
    }
    adjustSize();
    QTimer::singleShot(0, this, &DockWindow::positionDock);
    QTimer::singleShot(0, this, &DockWindow::applyOverlapPolicy);
    updateTargets();
    updateScales();
}

void DockWindow::onItemPointerMoved(const QPoint &globalPos) {
    if (!m_container) {
        return;
    }

    const QPoint local = m_container->mapFromGlobal(globalPos);
    m_cursorXInContainer = local.x();
    updateTargets();
}

void DockWindow::onItemClicked(int index) {
    if (index < 0 || index >= m_model.count()) {
        return;
    }

    const DockAppEntry app = m_model.apps().at(index);
    const QStringList args = QProcess::splitCommand(app.exec);
    if (args.isEmpty()) {
        qWarning() << "No exec command available for" << app.name;
        return;
    }

    const QString program = args.first();
    const QStringList programArgs = args.mid(1);

    if (!QProcess::startDetached(program, programArgs)) {
        qWarning() << "Failed to launch app:" << app.name << app.exec;
        return;
    }

    qDebug() << "Launched app:" << app.name << app.exec;
}

void DockWindow::onItemRemoveRequested(int index) {
    qDebug() << "Removing pinned app at index" << index;
    if (m_model.removeAt(index)) {
        syncModelToConfig();
        m_config.save();
    }
}

void DockWindow::tickAnimations() {
    const qint64 elapsedMs = m_elapsed.restart();
    const double dt = elapsedMs / 1000.0;
    const double duration = std::max(0.001, m_config.data().animationDurationMs / 1000.0);

    bool changed = false;
    for (DockItemWidget *item : m_itemWidgets) {
        changed |= item->tick(dt, duration);
    }

    if (!qFuzzyCompare(m_dodgeProgress, m_dodgeTarget)) {
        const double speed = dt / duration;
        if (m_dodgeProgress < m_dodgeTarget) {
            m_dodgeProgress = std::min(m_dodgeProgress + speed, m_dodgeTarget);
        } else {
            m_dodgeProgress = std::max(m_dodgeProgress - speed, m_dodgeTarget);
        }
        updateDockSlidePosition();
    }

    if (changed) {
        updateScales();
    }
}

void DockWindow::updateDodgeOverlapState() {
    if (m_config.data().overlapMode != "dodge") {
        m_dodgeTarget = 0.0;
        return;
    }

    if (!isVisible()) {
        return;
    }

#if KDEP6DOCK_HAS_X11
    if (m_isX11) {
        m_dodgeTarget = detectOverlapOnX11() ? 1.0 : 0.0;
        return;
    }
#endif

    if (m_isWayland) {
        qWarning() << "Dodge overlap detection is limited on Wayland for standalone clients; keeping dock visible.";
    }
    m_dodgeTarget = 0.0;
}

void DockWindow::updateTargets() {
    if (m_itemWidgets.isEmpty()) {
        return;
    }

    const bool cursorActive = m_cursorXInContainer >= 0.0;
    const double iconSpan = m_config.data().baseIconSize + m_config.data().spacing;
    const double influenceRadius = std::max(1.0, iconSpan * std::max(1, m_config.data().neighborRadius));

    for (DockItemWidget *item : m_itemWidgets) {
        double target = 0.0;
        if (cursorActive) {
            const double centerX = item->geometry().center().x();
            const double distance = std::abs(centerX - m_cursorXInContainer);
            const double normalized = distance / influenceRadius;
            target = std::exp(-(normalized * normalized));
        }
        item->setTargetProgress(target);
    }
}

void DockWindow::updateScales() {
    const double spread = std::max(0.0, m_config.data().maxScale - 1.0);
    for (DockItemWidget *item : m_itemWidgets) {
        const double scale = 1.0 + spread * item->progress();
        item->setCurrentScale(scale);
    }
}

void DockWindow::positionDock() {
    if (layout()) {
        layout()->activate();
    }
    if (width() <= 0 || height() <= 0) {
        adjustSize();
    }

    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        return;
    }

    const QRect screenGeometry = screen->geometry();
    m_baseX = screenGeometry.x() + (screenGeometry.width() - width()) / 2;
    m_baseY = screenGeometry.y() + screenGeometry.height() - height() - m_config.data().dockMarginBottom;
    updateDockSlidePosition();
}

void DockWindow::updateDockSlidePosition() {
    const int hiddenOffset = height() + m_config.data().dockMarginBottom + 2;
    const int y = m_baseY + qRound(hiddenOffset * m_dodgeProgress);
    move(m_baseX, y);
}

int DockWindow::insertionIndexForPos(const QPoint &pos) const {
    for (int i = 0; i < m_itemWidgets.size(); ++i) {
        const DockItemWidget *item = m_itemWidgets.at(i);
        if (pos.x() < item->geometry().center().x()) {
            return i;
        }
    }
    return m_itemWidgets.size();
}

bool DockWindow::addDesktopFile(const QString &path) {
    DockAppEntry entry;
    QString error;
    if (!DesktopEntry::parseFile(path, entry, error)) {
        qWarning() << "Failed to parse desktop file:" << path << error;
        return false;
    }

    qDebug() << "Adding desktop entry:" << entry.name << path;
    m_model.addApp(entry);
    return true;
}

void DockWindow::syncModelToConfig() {
    m_config.data().pinnedApps = m_model.apps();
}

void DockWindow::clearCursorInfluence() {
    if (m_cursorXInContainer < 0.0) {
        return;
    }

    m_cursorXInContainer = -1.0;
    updateTargets();
}

void DockWindow::applyShellWindowHints() {
    qDebug() << "Applying shell-like window hints (frameless, no-focus, tool/dock-like).";

#if KDEP6DOCK_HAS_X11
    if (!m_isX11) {
        return;
    }

    Display *display = XOpenDisplay(nullptr);
    if (!display) {
        qWarning() << "No X11 display found; skipping X11-specific shell hints.";
        return;
    }

    Window window = static_cast<Window>(winId());
    const Atom atomWindowType = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    const Atom atomWindowTypeDock = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False);
    XChangeProperty(display, window, atomWindowType, XA_ATOM, 32, PropModeReplace,
                    reinterpret_cast<const unsigned char *>(&atomWindowTypeDock), 1);
    XFlush(display);
    XCloseDisplay(display);
    qDebug() << "Applied _NET_WM_WINDOW_TYPE_DOCK on X11.";
#else
    if (m_isX11) {
        qWarning() << "Built without X11 headers; advanced dock hints unavailable on X11.";
    }
#endif
}

void DockWindow::applyOverlapPolicy() {
    const QString mode = m_config.data().overlapMode;

    if (mode == "ignore") {
        qDebug() << "Overlap mode: ignore (overlay behavior, no reserved desktop space).";
        m_dodgeTarget = 0.0;
#if KDEP6DOCK_HAS_X11
        if (m_isX11) {
            Display *display = XOpenDisplay(nullptr);
            if (!display) {
                return;
            }
            Window window = static_cast<Window>(winId());
            const Atom atomStrut = XInternAtom(display, "_NET_WM_STRUT", False);
            const Atom atomStrutPartial = XInternAtom(display, "_NET_WM_STRUT_PARTIAL", False);
            unsigned long strut[4] = {0, 0, 0, 0};
            unsigned long strutPartial[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
            XChangeProperty(display, window, atomStrut, XA_CARDINAL, 32, PropModeReplace,
                            reinterpret_cast<const unsigned char *>(strut), 4);
            XChangeProperty(display, window, atomStrutPartial, XA_CARDINAL, 32, PropModeReplace,
                            reinterpret_cast<const unsigned char *>(strutPartial), 12);
            XFlush(display);
            XCloseDisplay(display);
        }
#endif
        return;
    }

    if (mode == "dodge") {
        qDebug() << "Overlap mode: dodge (hide dock when overlapped by normal windows).";
#if KDEP6DOCK_HAS_X11
        if (m_isX11) {
            updateDodgeOverlapState();
            return;
        }
#endif
        qWarning() << "Dodge mode currently implemented for X11/XWayland; fallback to ignore-like behavior on this platform.";
        m_dodgeTarget = 0.0;
        return;
    }

    if (mode != "block") {
        qWarning() << "Unknown overlap mode:" << mode << "(using ignore behavior).";
        m_dodgeTarget = 0.0;
        return;
    }

    m_dodgeTarget = 0.0;

#if KDEP6DOCK_HAS_X11
    if (m_isX11) {
        Display *display = XOpenDisplay(nullptr);
        if (!display) {
            qWarning() << "X11 display unavailable; cannot apply block overlap reservations.";
            return;
        }
        Window window = static_cast<Window>(winId());
        QScreen *screen = QGuiApplication::primaryScreen();
        if (!screen) {
            XCloseDisplay(display);
            return;
        }

        const QRect g = screen->geometry();
        const unsigned long reserve = static_cast<unsigned long>(height() + m_config.data().dockMarginBottom);

        unsigned long strut[4] = {0, 0, 0, reserve};
        unsigned long strutPartial[12] = {
            0, 0, 0, reserve,
            0, 0, 0, 0,
            0, 0,
            static_cast<unsigned long>(std::max(0, g.x())),
            static_cast<unsigned long>(std::max(0, g.x() + g.width() - 1))
        };

        const Atom atomStrut = XInternAtom(display, "_NET_WM_STRUT", False);
        const Atom atomStrutPartial = XInternAtom(display, "_NET_WM_STRUT_PARTIAL", False);
        XChangeProperty(display, window, atomStrut, XA_CARDINAL, 32, PropModeReplace,
                        reinterpret_cast<const unsigned char *>(strut), 4);
        XChangeProperty(display, window, atomStrutPartial, XA_CARDINAL, 32, PropModeReplace,
                        reinterpret_cast<const unsigned char *>(strutPartial), 12);
        XFlush(display);
        XCloseDisplay(display);

        qDebug() << "Overlap mode: block (X11 strut reservation applied; may vary by WM/compositor).";
        return;
    }
#endif

    if (m_isWayland) {
        qWarning() << "Overlap mode 'block' requested under Wayland. Standalone Qt windows cannot reliably reserve screen space; using overlay-like behavior.";
    } else {
        qWarning() << "Overlap mode 'block' unsupported on this platform/build; using overlay-like behavior.";
    }
}

bool DockWindow::detectOverlapOnX11() const {
#if !KDEP6DOCK_HAS_X11
    return false;
#else
    Display *display = XOpenDisplay(nullptr);
    if (!display) {
        return false;
    }

    const QRect dockRect(frameGeometry());
    const Window selfWindow = static_cast<Window>(winId());

    Window root = DefaultRootWindow(display);
    Window rootReturned = 0;
    Window parentReturned = 0;
    Window *children = nullptr;
    unsigned int childCount = 0;

    if (!XQueryTree(display, root, &rootReturned, &parentReturned, &children, &childCount)) {
        XCloseDisplay(display);
        return false;
    }

    const Atom atomWindowType = XInternAtom(display, "_NET_WM_WINDOW_TYPE", True);
    const Atom atomDock = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", True);
    const Atom atomDesktop = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DESKTOP", True);
    const Atom atomUtility = XInternAtom(display, "_NET_WM_WINDOW_TYPE_UTILITY", True);
    const Atom atomToolbar = XInternAtom(display, "_NET_WM_WINDOW_TYPE_TOOLBAR", True);
    const Atom atomMenu = XInternAtom(display, "_NET_WM_WINDOW_TYPE_MENU", True);
    const Atom atomPopupMenu = XInternAtom(display, "_NET_WM_WINDOW_TYPE_POPUP_MENU", True);
    const Atom atomDropdown = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU", True);
    const Atom atomNotification = XInternAtom(display, "_NET_WM_WINDOW_TYPE_NOTIFICATION", True);

    bool overlaps = false;

    for (unsigned int i = 0; i < childCount && !overlaps; ++i) {
        const Window w = children[i];
        if (w == selfWindow) {
            continue;
        }

        XWindowAttributes attr{};
        if (!XGetWindowAttributes(display, w, &attr) || attr.map_state != IsViewable) {
            continue;
        }

        bool skipWindow = false;
        if (atomWindowType != None) {
            Atom actualType = None;
            int actualFormat = 0;
            unsigned long nItems = 0;
            unsigned long bytesAfter = 0;
            unsigned char *prop = nullptr;
            if (XGetWindowProperty(display, w, atomWindowType, 0, 8, False, XA_ATOM,
                                   &actualType, &actualFormat, &nItems, &bytesAfter, &prop) == Success && prop) {
                const Atom *types = reinterpret_cast<const Atom *>(prop);
                for (unsigned long j = 0; j < nItems; ++j) {
                    const Atom t = types[j];
                    if (t == atomDock || t == atomDesktop || t == atomUtility || t == atomToolbar ||
                        t == atomMenu || t == atomPopupMenu || t == atomDropdown || t == atomNotification) {
                        skipWindow = true;
                        break;
                    }
                }
                XFree(prop);
            }
        }

        if (skipWindow) {
            continue;
        }

        int absX = 0;
        int absY = 0;
        Window child = 0;
        if (!XTranslateCoordinates(display, w, root, 0, 0, &absX, &absY, &child)) {
            continue;
        }

        QRect windowRect(absX, absY, attr.width, attr.height);
        if (windowRect.isValid() && windowRect.intersects(dockRect)) {
            overlaps = true;
        }
    }

    if (children) {
        XFree(children);
    }
    XCloseDisplay(display);
    return overlaps;
#endif
}
