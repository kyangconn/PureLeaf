import QtQuick

Rectangle {
    id: root

    default property alias content: body.data
    property var theme: controller.theme
    property int padding: 16

    color: theme.surface
    radius: theme.radius
    border.color: theme.border
    border.width: 1

    implicitWidth: body.implicitWidth + padding * 2
    implicitHeight: body.implicitHeight + padding * 2

    Item {
        id: body
        anchors.fill: parent
        anchors.margins: root.padding
    }
}
