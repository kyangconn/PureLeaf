import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import QWindowKit

Item {
    id: root

    property var theme: controller.theme
    property var window
    property var agent

    signal settingsRequested()

    height: theme.titleBarHeight

    Component.onCompleted: {
        if (!agent) return
        agent.setTitleBar(root)
        agent.setHitTestVisible(settingsButton, true)
        agent.setHitTestVisible(minimizeButton, true)
        agent.setHitTestVisible(maximizeButton, true)
        agent.setHitTestVisible(closeButton, true)
        agent.setSystemButton(WindowAgent.Minimize, minimizeButton)
        agent.setSystemButton(WindowAgent.Maximize, maximizeButton)
        agent.setSystemButton(WindowAgent.Close, closeButton)
    }

    Rectangle {
        anchors.fill: parent
        color: theme.surface
        border.color: theme.border
        border.width: 1
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 6
        spacing: 8

        Icon {
            name: "leaf"
            size: 18
            color: theme.accent
            Layout.alignment: Qt.AlignVCenter
        }

        Label {
            text: window ? window.title : "PureLeaf"
            color: theme.fg
            font.weight: Font.DemiBold
            elide: Text.ElideRight
            Layout.fillWidth: true
        }

        PLIconButton {
            id: settingsButton
            iconName: "settings"
            tooltip: qsTr("设置")
            theme: root.theme
            onClicked: root.settingsRequested()
        }

        PLIconButton {
            id: minimizeButton
            iconName: "minus"
            tooltip: qsTr("最小化")
            theme: root.theme
            onClicked: if (root.window) root.window.showMinimized()
        }

        PLIconButton {
            id: maximizeButton
            iconName: root.window && root.window.visibility === Window.Maximized ? "copy" : "square"
            tooltip: root.window && root.window.visibility === Window.Maximized ? qsTr("还原") : qsTr("最大化")
            theme: root.theme
            onClicked: {
                if (root.window) {
                    if (root.window.visibility === Window.Maximized) {
                        root.window.showNormal()
                    } else {
                        root.window.showMaximized()
                    }
                }
            }
        }

        PLIconButton {
            id: closeButton
            iconName: "x"
            tooltip: qsTr("关闭")
            danger: true
            theme: root.theme
            onClicked: if (root.window) root.window.close()
        }
    }
}
