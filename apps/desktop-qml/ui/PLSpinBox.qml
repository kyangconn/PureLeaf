import QtQuick
import QtQuick.Controls

SpinBox {
    id: root

    property var theme: controller.theme

    implicitWidth: 120
    implicitHeight: 36
    editable: true

    contentItem: TextInput {
        z: 2
        text: root.textFromValue(root.value, root.locale)
        font: root.font
        color: theme.fg
        selectionColor: theme.accentSoft
        selectedTextColor: theme.fg
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        readOnly: !root.editable
        validator: root.validator
        inputMethodHints: Qt.ImhFormattedNumbersOnly
        onEditingFinished: root.value = root.valueFromText(text, root.locale)
    }

    up.indicator: Rectangle {
        x: root.width - width - 2
        y: 2
        width: 28
        height: (root.height - 4) / 2
        radius: 6
        color: root.up.pressed ? theme.surface : root.up.hovered ? theme.surfaceAlt : "transparent"

        Text {
            anchors.centerIn: parent
            text: "+"
            color: theme.fgMuted
            font.pixelSize: 12
        }
    }

    down.indicator: Rectangle {
        x: root.width - width - 2
        y: root.height / 2
        width: 28
        height: (root.height - 4) / 2
        radius: 6
        color: root.down.pressed ? theme.surface : root.down.hovered ? theme.surfaceAlt : "transparent"

        Text {
            anchors.centerIn: parent
            text: "-"
            color: theme.fgMuted
            font.pixelSize: 12
        }
    }

    background: Rectangle {
        radius: theme.radius
        color: theme.surfaceAlt
        border.color: root.activeFocus ? theme.accent : theme.border
        border.width: 1
    }
}
