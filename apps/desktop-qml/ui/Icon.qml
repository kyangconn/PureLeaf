import QtQuick
import QtQuick.Effects

Item {
    id: root

    property string name
    property color color: controller.theme.fg
    property color hoverColor: color
    property bool hovered: false
    property int size: 18

    implicitWidth: size
    implicitHeight: size
    width: size
    height: size

    Image {
        id: iconSource
        anchors.fill: parent
        visible: false
        mipmap: true
        fillMode: Image.PreserveAspectFit
        source: root.name.length > 0 ? "qrc:/pureleaf/icons/lucide/" + root.name + ".svg" : ""
    }

    MultiEffect {
        anchors.fill: iconSource
        source: iconSource
        colorization: 1
        colorizationColor: root.hovered ? root.hoverColor : root.color
        visible: iconSource.status === Image.Ready
    }
}
