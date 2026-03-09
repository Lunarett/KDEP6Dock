#include "DockWindow.h"

#include "DesktopEntry.h"

#include <QApplication>
#include <QCursor>
#include <algorithm>
#include <QDragEnterEvent>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <QProcess>
#include <QScreen>
#include <QUrl>

DockWindow::DockWindow(QWidget *parent)
    : QWidget(parent) {
    setAcceptDrops(true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);

    m_config.load();
    m_model.setApps(m_config.data().pinnedApps);

    auto *outerLayout = new QHBoxLayout(this);
    outerLayout->setContentsMargins(m_config.data().dockPadding, m_config.data().dockPadding,
                                    m_config.data().dockPadding, m_config.data().dockPadding);

    m_container = new QWidget(this);
    m_layout = new QHBoxLayout(m_container);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(m_config.data().spacing);
    outerLayout->addWidget(m_container);

    connect(&m_model, &DockModel::modelChanged, this, &DockWindow::rebuildUi);
    connect(&m_animationTimer, &QTimer::timeout, this, &DockWindow::tickAnimations);

    m_animationTimer.start(16);
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
        to = qBound(0, to, m_model.count() - 1);
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

void DockWindow::rebuildUi() {
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

        connect(item, &DockItemWidget::hovered, this, &DockWindow::onHovered);
        connect(item, &DockItemWidget::unhovered, this, &DockWindow::onUnhovered);
        connect(item, &DockItemWidget::clicked, this, &DockWindow::onItemClicked);
        connect(item, &DockItemWidget::removeRequested, this, &DockWindow::onItemRemoveRequested);

        m_layout->addWidget(item);
        m_itemWidgets.push_back(item);
        ++index;
    }

    adjustSize();
    positionDock();
    updateTargets();
    updateScales();
}

void DockWindow::onHovered(int index) {
    m_hoveredIndex = index;
    updateTargets();
}

void DockWindow::onUnhovered(int index) {
    Q_UNUSED(index)
    QPoint globalPos = QCursor::pos();
    if (!frameGeometry().contains(globalPos)) {
        m_hoveredIndex = -1;
    } else {
        QWidget *child = childAt(mapFromGlobal(globalPos));
        DockItemWidget *item = nullptr;
        while (child && !item) {
            item = qobject_cast<DockItemWidget *>(child);
            child = child->parentWidget();
        }
        m_hoveredIndex = item ? item->index() : -1;
    }
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

    if (changed) {
        updateScales();
    }
}

void DockWindow::updateTargets() {
    for (DockItemWidget *item : m_itemWidgets) {
        const int distance = std::abs(item->index() - m_hoveredIndex);
        double target = 0.0;
        if (m_hoveredIndex >= 0 && distance <= m_config.data().neighborRadius) {
            const double radius = std::max(1, m_config.data().neighborRadius);
            target = 1.0 - (distance / radius);
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
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        return;
    }

    const QRect g = screen->availableGeometry();
    const int x = g.x() + (g.width() - width()) / 2;
    const int y = g.bottom() - height() - m_config.data().dockMarginBottom;
    move(x, y);
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
