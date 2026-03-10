#pragma once

#include "Config.h"
#include "DockItemWidget.h"
#include "DockModel.h"

#include <QElapsedTimer>
#include <QHBoxLayout>
#include <QTimer>
#include <QWidget>

class DockWindow : public QWidget {
    Q_OBJECT
public:
    explicit DockWindow(QWidget *parent = nullptr);
    ~DockWindow() override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void requestRebuildUi();
    void rebuildUi();
    void onItemPointerMoved(const QPoint &globalPos);
    void onItemClicked(int index);
    void onItemRemoveRequested(int index);
    void tickAnimations();
    void updateDodgeOverlapState();

private:
    void updateTargets();
    void updateScales();
    void positionDock();
    int insertionIndexForPos(const QPoint &pos) const;
    bool addDesktopFile(const QString &path);
    void syncModelToConfig();
    void clearCursorInfluence();
    void applyShellWindowHints();
    void applyOverlapPolicy();
    void updateDockSlidePosition();
    bool detectOverlapOnX11() const;

    Config m_config;
    DockModel m_model;
    QVector<DockItemWidget *> m_itemWidgets;
    QWidget *m_container = nullptr;
    QHBoxLayout *m_layout = nullptr;
    QTimer m_animationTimer;
    QTimer m_overlapCheckTimer;
    QElapsedTimer m_elapsed;
    double m_cursorXInContainer = -1.0;
    bool m_rebuildQueued = false;
    bool m_isX11 = false;
    bool m_isWayland = false;
    int m_baseX = 0;
    int m_baseY = 0;
    double m_dodgeProgress = 0.0;
    double m_dodgeTarget = 0.0;
};
