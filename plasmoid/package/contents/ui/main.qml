import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.taskmanager as TaskManager

Item {
    id: root

    readonly property int baseIconSize: plasmoid.configuration.baseIconSize || 48
    readonly property real maxScale: plasmoid.configuration.maxScale || 1.6
    readonly property int spacing: plasmoid.configuration.spacing || 10
    readonly property int animationDurationMs: plasmoid.configuration.animationDurationMs || 180
    readonly property int neighborRadius: plasmoid.configuration.neighborRadius || 2
    readonly property int dockPadding: plasmoid.configuration.dockPadding || 12
    readonly property real backgroundOpacity: plasmoid.configuration.backgroundOpacity || 0.65
    readonly property int cornerRadius: plasmoid.configuration.cornerRadius || 18

    property real pointerX: -1

    implicitHeight: dockBackground.implicitHeight
    implicitWidth: dockBackground.implicitWidth

    TaskManager.TasksModel {
        id: tasksModel

        sortMode: TaskManager.TasksModel.SortManual
        groupMode: TaskManager.TasksModel.GroupDisabled
        separateLaunchers: true
        launchInPlace: true
    }

    Rectangle {
        id: dockBackground
        anchors.centerIn: parent
        radius: root.cornerRadius
        color: Qt.rgba(0.11, 0.11, 0.12, root.backgroundOpacity)
        implicitHeight: row.implicitHeight + root.dockPadding * 2
        implicitWidth: row.implicitWidth + root.dockPadding * 2

        Row {
            id: row
            anchors.centerIn: parent
            spacing: root.spacing

            Repeater {
                id: itemRepeater
                model: tasksModel

                delegate: Item {
                    id: iconSlot

                    readonly property real centerX: x + width / 2
                    readonly property real influenceRadiusPx: Math.max(1, (root.baseIconSize + root.spacing) * Math.max(1, root.neighborRadius))
                    readonly property real normalizedDistance: root.pointerX < 0 ? 10 : Math.abs((root.pointerX - row.x) - centerX) / influenceRadiusPx
                    readonly property real targetProgress: root.pointerX < 0 ? 0 : Math.exp(-Math.pow(normalizedDistance, 2))
                    readonly property real currentScale: 1 + (root.maxScale - 1) * progress

                    property real progress: 0

                    width: Math.round(root.baseIconSize * root.maxScale) + 8
                    height: width

                    Behavior on progress {
                        NumberAnimation {
                            duration: root.animationDurationMs
                            easing.type: Easing.InOutQuad
                        }
                    }

                    onTargetProgressChanged: progress = targetProgress

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 2
                        radius: 12
                        color: Qt.rgba(1, 1, 1, (progress * 35) / 255)
                    }

                    PlasmaCore.IconItem {
                        id: icon
                        anchors.centerIn: parent
                        width: Math.round(root.baseIconSize * iconSlot.currentScale)
                        height: width
                        source: model.Decoration
                        smooth: true
                    }

                    PlasmaComponents3.ToolTip {
                        text: model.AppName || model.display || "Application"
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton

                        onClicked: (mouse) => {
                            const idx = tasksModel.makeModelIndex(index);
                            if (mouse.button === Qt.LeftButton) {
                                tasksModel.requestActivate(idx);
                            } else if (mouse.button === Qt.MiddleButton) {
                                tasksModel.requestClose(idx);
                            }
                        }
                    }
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.NoButton
            propagateComposedEvents: true

            onPositionChanged: (mouse) => root.pointerX = mouse.x
            onExited: root.pointerX = -1
        }
    }
}
