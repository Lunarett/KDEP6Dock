#include "DockItemWidget.h"

#include <QApplication>
#include <algorithm>
#include <QContextMenuEvent>
#include <QDrag>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>

DockItemWidget::DockItemWidget(const DockAppEntry &entry, int index, QWidget *parent)
    : QWidget(parent),
      m_entry(entry),
      m_index(index),
      m_icon(QIcon::fromTheme(entry.icon)) {
    if (m_icon.isNull() && !entry.icon.isEmpty()) {
        m_icon = QIcon(entry.icon);
    }
    setToolTip(entry.name);
    setMouseTracking(true);
}

void DockItemWidget::setIndex(int index) {
    m_index = index;
}

int DockItemWidget::index() const {
    return m_index;
}

void DockItemWidget::setBaseIconSize(int size) {
    m_baseIconSize = size;
}

void DockItemWidget::setMaxScale(double maxScale) {
    m_maxScale = maxScale;
}

void DockItemWidget::setTargetProgress(double target) {
    m_target = std::clamp(target, 0.0, 1.0);
}

double DockItemWidget::progress() const {
    return m_progress;
}

bool DockItemWidget::tick(double dtSeconds, double durationSeconds) {
    if (durationSeconds <= 0.0) {
        m_progress = m_target;
        return true;
    }

    const double speed = dtSeconds / durationSeconds;
    if (qFuzzyCompare(m_progress, m_target)) {
        m_progress = m_target;
        return false;
    }

    if (m_progress < m_target) {
        m_progress = std::min(m_progress + speed, m_target);
    } else {
        m_progress = std::max(m_progress - speed, m_target);
    }
    update();
    return true;
}

void DockItemWidget::setCurrentScale(double scale) {
    m_currentScale = std::clamp(scale, 1.0, m_maxScale);
    updateGeometry();
    update();
}

void DockItemWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_pressed = true;
        m_dragging = false;
        m_pressPos = event->pos();
    }
    QWidget::mousePressEvent(event);
}

void DockItemWidget::mouseMoveEvent(QMouseEvent *event) {
    emit pointerMovedGlobal(mapToGlobal(event->pos()));

    if (!m_pressed || !(event->buttons() & Qt::LeftButton)) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    if ((event->pos() - m_pressPos).manhattanLength() < QApplication::startDragDistance()) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    m_dragging = true;
    auto *drag = new QDrag(this);
    auto *mime = new QMimeData();
    mime->setData("application/x-kdep6dock-index", QByteArray::number(m_index));
    drag->setMimeData(mime);
    drag->exec(Qt::MoveAction);
    m_dragging = false;
    m_pressed = false;
}

void DockItemWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_pressed) {
        if (!m_dragging && (event->pos() - m_pressPos).manhattanLength() < QApplication::startDragDistance()) {
            emit clicked(m_index);
        }
        m_pressed = false;
        m_dragging = false;
    }
    QWidget::mouseReleaseEvent(event);
}

void DockItemWidget::contextMenuEvent(QContextMenuEvent *event) {
    QMenu menu(this);
    QAction *remove = menu.addAction(tr("Remove from Dock"));
    QAction *picked = menu.exec(event->globalPos());
    if (picked == remove) {
        emit removeRequested(m_index);
    }
}

void DockItemWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const int side = qRound(m_baseIconSize * m_currentScale);
    const QRect iconRect((width() - side) / 2, (height() - side) / 2, side, side);

    QColor hoverColor(255, 255, 255, static_cast<int>(35 * m_progress));
    painter.setBrush(hoverColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect().adjusted(2, 2, -2, -2), 12, 12);

    m_icon.paint(&painter, iconRect);
}
