import QtQuick
import QtQuick.Controls

TextField {
    id: root

    property var theme: controller.theme

    implicitHeight: 36
    leftPadding: 12
    rightPadding: 12
    color: theme.fg
    placeholderTextColor: theme.fgMuted
    selectionColor: theme.accentSoft
    selectedTextColor: theme.fg

    background: Rectangle {
        radius: theme.radius
        color: root.activeFocus ? theme.surface : theme.surfaceAlt
        border.color: root.activeFocus ? theme.accent : theme.border
        border.width: 1
    }
}
