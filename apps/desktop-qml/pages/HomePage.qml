import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Item {
    id: home

    property var theme: controller.theme

    Dialog {
        id: newProjectDialog
        anchors.centerIn: parent
        modal: true
        title: qsTr("新建项目")
        standardButtons: Dialog.Ok | Dialog.Cancel
        background: Rectangle {
            color: theme.surface
            radius: theme.radius
            border.color: theme.border
            border.width: 1
        }

        onOpened: nameInput.clear()
        onAccepted: {
            if (nameInput.text.trim().length > 0) {
                controller.createBlankProject(nameInput.text)
            }
        }

        contentItem: ColumnLayout {
            spacing: 10

            Label {
                text: qsTr("项目名称")
                color: theme.fgMuted
            }

            PLTextField {
                id: nameInput
                Layout.preferredWidth: 320
                theme: home.theme
                placeholderText: qsTr("例如：我的论文")
                focus: true
                onAccepted: newProjectDialog.accept()
            }
        }
    }

    FolderDialog {
        id: openFolderDialog
        title: qsTr("选择项目文件夹")
        currentFolder: "file:///" + controller.userDataDir.replace(/\\/g, "/")
        onAccepted: controller.openFolder(selectedFolder)
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: 24

            Item {
                Layout.fillHeight: true
                Layout.preferredHeight: controller.recentProjects.length > 0 ? 26 : 46
            }

            Icon {
                name: "leaf"
                size: 42
                color: theme.accent
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: "PureLeaf"
                font.pixelSize: 38
                font.weight: Font.DemiBold
                color: theme.fg
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("本地优先的 LaTeX 写作空间")
                font.pixelSize: 14
                color: theme.fgMuted
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 10
                spacing: 12

                PLButton {
                    text: qsTr("新建项目")
                    iconName: "file-plus"
                    variant: "primary"
                    theme: home.theme
                    Layout.preferredWidth: controller.recentProjects.length > 0 ? 160 : 220
                    Layout.preferredHeight: 42
                    onClicked: newProjectDialog.open()
                }

                PLButton {
                    text: qsTr("打开文件夹")
                    iconName: "folder-open"
                    theme: home.theme
                    Layout.preferredWidth: controller.recentProjects.length > 0 ? 160 : 220
                    Layout.preferredHeight: 42
                    onClicked: openFolderDialog.open()
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 56
                Layout.rightMargin: 56
                Layout.topMargin: 22
                spacing: 10
                visible: controller.recentProjects.length > 0

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: qsTr("最近项目")
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                        color: theme.fg
                        Layout.fillWidth: true
                    }
                    Label {
                        text: controller.recentProjects.length + qsTr(" 个")
                        color: theme.fgMuted
                    }
                }

                Repeater {
                    model: controller.recentProjects

                    delegate: PLCard {
                        theme: home.theme
                        Layout.fillWidth: true
                        Layout.preferredHeight: 78
                        color: cardMouse.containsMouse ? theme.surfaceAlt : theme.surface

                        MouseArea {
                            id: cardMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: controller.openRecentProject(modelData.id)
                        }

                        RowLayout {
                            anchors.fill: parent
                            spacing: 12

                            Rectangle {
                                Layout.preferredWidth: 40
                                Layout.preferredHeight: 40
                                radius: 8
                                color: theme.accentSoft

                                Icon {
                                    anchors.centerIn: parent
                                    name: "leaf"
                                    size: 18
                                    color: theme.accent
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Label {
                                    Layout.fillWidth: true
                                    text: modelData.name
                                    color: theme.fg
                                    font.weight: Font.DemiBold
                                    elide: Text.ElideRight
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: modelData.rootPath
                                    color: theme.fgMuted
                                    font.pixelSize: 12
                                    elide: Text.ElideMiddle
                                }
                            }

                            Label {
                                text: modelData.mainTex || qsTr("未设置主文件")
                                color: theme.fgMuted
                                font.pixelSize: 12
                            }

                            PLIconButton {
                                iconName: "trash-2"
                                tooltip: qsTr("移除")
                                theme: home.theme
                                onClicked: controller.removeRecentProject(modelData.id)
                            }
                        }
                    }
                }
            }

            Item {
                Layout.fillHeight: true
                Layout.preferredHeight: 40
            }
        }
    }
}
