#pragma once

#include "DockModel.h"

#include <QElapsedTimer>
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
    void hovered(int index);
    void unhovered(int index);
    void clicked(int index);
    void removeRequested(int index);
    void dragStarted();

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
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
    QIcon m_icon;
};
