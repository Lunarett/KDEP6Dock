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

private slots:
    void rebuildUi();
    void onHovered(int index);
    void onUnhovered(int index);
    void onItemClicked(int index);
    void onItemRemoveRequested(int index);
    void tickAnimations();

private:
    void updateTargets();
    void updateScales();
    void positionDock();
    int insertionIndexForPos(const QPoint &pos) const;
    bool addDesktopFile(const QString &path);
    void syncModelToConfig();

    Config m_config;
    DockModel m_model;
    QVector<DockItemWidget *> m_itemWidgets;
    QWidget *m_container = nullptr;
    QHBoxLayout *m_layout = nullptr;
    QTimer m_animationTimer;
    QElapsedTimer m_elapsed;
    int m_hoveredIndex = -1;
};
