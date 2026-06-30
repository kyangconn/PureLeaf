import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
    id: root

    property var theme: controller.theme
    property string variant: "secondary"
    property string iconName: ""

    implicitHeight: 36
    leftPadding: 14
    rightPadding: 14
    topPadding: 0
    bottomPadding: 0
    spacing: 8

    function baseColor() {
        if (variant === "primary") return theme.accent
        if (variant === "danger") return theme.danger
        if (variant === "ghost") return "transparent"
        return theme.surface
    }

    function hoverColor() {
        if (variant === "primary") return Qt.lighter(theme.accent, 1.08)
        if (variant === "danger") return Qt.lighter(theme.danger, 1.08)
        if (variant === "ghost") return theme.surfaceAlt
        return theme.surfaceAlt
    }

    function pressColor() {
        if (variant === "primary") return Qt.darker(theme.accent, 1.12)
        if (variant === "danger") return Qt.darker(theme.danger, 1.12)
        return theme.surfaceAlt
    }

    function foregroundColor() {
        if (variant === "primary" || variant === "danger") return theme.surface
        if (!enabled) return theme.fgMuted
        return theme.fg
    }

    contentItem: RowLayout {
        spacing: root.spacing

        Icon {
            visible: root.iconName.length > 0
            name: root.iconName
            size: 16
            color: root.foregroundColor()
            hovered: root.hovered
            Layout.alignment: Qt.AlignVCenter
        }

        Label {
            text: root.text
            color: root.foregroundColor()
            font.weight: root.variant === "primary" ? Font.DemiBold : Font.Medium
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
    }

    background: Rectangle {
        radius: theme.radius
        color: !root.enabled ? theme.surfaceAlt
                             : root.down ? root.pressColor()
                                         : root.hovered ? root.hoverColor()
                                                        : root.baseColor()
        border.color: root.variant === "primary" || root.variant === "danger"
                      ? "transparent"
                      : theme.border
        border.width: root.variant === "primary" || root.variant === "danger" ? 0 : 1
    }
}
