import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import QWindowKit

ApplicationWindow {
    id: root

    width: controller.windowProfile.width
    height: controller.windowProfile.height
    minimumWidth: controller.windowProfile.minWidth
    minimumHeight: controller.windowProfile.minHeight
    visible: false
    title: qsTr("PureLeaf")
    color: theme.bg

    Theme {
        id: theme
        source: controller.theme
    }

    WindowAgent {
        id: windowAgent
    }

    Component.onCompleted: {
        windowAgent.setup(root)
        applyWindowProfile(true)
        visible = true
    }

    function applyWindowProfile(forceResize) {
        const profile = controller.windowProfile
        minimumWidth = profile.minWidth
        minimumHeight = profile.minHeight
        if (!forceResize && (visibility === Window.Maximized || visibility === Window.FullScreen)) {
            return
        }
        width = profile.width
        height = profile.height
    }

    Connections {
        target: controller
        function onWindowProfileChanged() {
            root.applyWindowProfile(false)
        }
        function onInfoMessage(title, message) {
            toast.show(title, message)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TitleBar {
            Layout.fillWidth: true
            Layout.preferredHeight: theme.titleBarHeight
            theme: theme
            window: root
            agent: windowAgent
            onSettingsRequested: controller.switchTo("settings")
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            HomePage {
                anchors.fill: parent
                theme: theme
                visible: controller.currentPage === "home"
                z: visible ? 1 : 0
            }

            EditorPage {
                anchors.fill: parent
                theme: theme
                visible: controller.currentPage === "editor"
                z: visible ? 1 : 0
            }

            SettingsPage {
                anchors.fill: parent
                theme: theme
                visible: controller.currentPage === "settings"
                z: visible ? 1 : 0
            }
        }
    }

    PLCard {
        id: toast
        z: 100
        width: Math.min(420, root.width - 48)
        height: toastColumn.implicitHeight + padding * 2
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 24
        theme: theme
        visible: opacity > 0
        opacity: 0
        padding: 12

        function show(title, message) {
            toastTitle.text = title
            toastMessage.text = message
            opacity = 1
            hideTimer.restart()
        }

        Timer {
            id: hideTimer
            interval: 3200
            onTriggered: toast.opacity = 0
        }

        Behavior on opacity { NumberAnimation { duration: 180 } }

        ColumnLayout {
            id: toastColumn
            anchors.fill: parent
            spacing: 2

            Label {
                id: toastTitle
                color: theme.fg
                font.bold: true
                Layout.fillWidth: true
            }

            Label {
                id: toastMessage
                color: theme.fgMuted
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
        }
    }
}
