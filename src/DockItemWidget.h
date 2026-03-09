#pragma once

#include "DockModel.h"

#include <QIcon>
#include <QWidget>

class DockItemWidget : public QWidget {
    Q_OBJECT
public:
    explicit DockItemWidget(const DockAppEntry &entry, int index, QWidget *parent = nullptr);

    void setIndex(int index);
    int index() const;

    void setBaseIconSize(int size);
    void setMaxScale(double maxScale);

    void setTargetProgress(double target);
    double progress() const;
    bool tick(double dtSeconds, double durationSeconds);

    void setCurrentScale(double scale);

signals:
    void pointerMovedGlobal(const QPoint &globalPos);
    void clicked(int index);
    void removeRequested(int index);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    DockAppEntry m_entry;
    int m_index = -1;
    int m_baseIconSize = 48;
    double m_maxScale = 1.5;
    double m_progress = 0.0;
    double m_target = 0.0;
    double m_currentScale = 1.0;
    QPoint m_pressPos;
    bool m_pressed = false;
    bool m_dragging = false;
    QIcon m_icon;
};
