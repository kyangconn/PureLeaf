import QtQuick
import QtQuick.Controls

Button {
    id: root

    property var theme: controller.theme
    property string iconName
    property string tooltip: ""
    property bool danger: false

    implicitWidth: 36
    implicitHeight: 36
    padding: 0
    focusPolicy: Qt.NoFocus

    ToolTip.text: tooltip
    ToolTip.visible: hovered && tooltip.length > 0
    ToolTip.delay: 450

    contentItem: Icon {
        anchors.centerIn: parent
        name: root.iconName
        size: 17
        color: root.danger && root.hovered ? theme.surface : theme.fgMuted
        hoverColor: root.danger ? theme.surface : theme.fg
        hovered: root.hovered
    }

    background: Rectangle {
        radius: theme.radius
        color: root.danger && root.hovered ? theme.danger
                                           : root.down || root.hovered ? theme.surfaceAlt
                                                                      : "transparent"
        border.color: root.danger && root.hovered ? "transparent" : theme.border
        border.width: root.danger && root.hovered ? 0 : 1
    }
}
