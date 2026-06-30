import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: editor

    property var theme: controller.theme
    property var project: controller.currentProject
    property bool syncingEditorText: false

    function syncEditorText() {
        syncingEditorText = true
        sourceEditor.text = controller.currentFileContent
        syncingEditorText = false
    }

    Component.onCompleted: syncEditorText()

    Connections {
        target: controller
        function onCurrentFileContentChanged() {
            editor.syncEditorText()
        }
    }

    Shortcut {
        sequence: StandardKey.Save
        onActivated: controller.saveCurrentFile()
    }

    Shortcut {
        sequence: "F5"
        onActivated: controller.compileCurrentProject()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            color: theme.surface
            border.color: theme.border
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 10

                PLIconButton {
                    iconName: "arrow-left"
                    tooltip: qsTr("返回首页")
                    theme: editor.theme
                    onClicked: controller.backHome()
                }

                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    Layout.topMargin: 12
                    Layout.bottomMargin: 12
                    color: theme.border
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    Label {
                        Layout.fillWidth: true
                        text: project.name || qsTr("未打开项目")
                        color: theme.fg
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: project.rootPath || controller.userDataDir
                        color: theme.fgMuted
                        font.pixelSize: 12
                        elide: Text.ElideMiddle
                    }
                }

                PLButton {
                    text: controller.currentFileRelativePath || qsTr("主文件")
                    iconName: "leaf"
                    variant: "ghost"
                    theme: editor.theme
                    Layout.preferredWidth: 190
                }

                PLButton {
                    text: controller.editorDirty ? qsTr("保存*") : qsTr("保存")
                    iconName: "copy"
                    theme: editor.theme
                    enabled: controller.currentFileRelativePath.length > 0
                    Layout.preferredWidth: 92
                    onClicked: controller.saveCurrentFile()
                }

                PLButton {
                    text: controller.compiling ? qsTr("编译中") : qsTr("编译")
                    iconName: "file-plus"
                    variant: "primary"
                    theme: editor.theme
                    enabled: !controller.compiling && project.id
                    Layout.preferredWidth: 102
                    onClicked: controller.compileCurrentProject()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 1

            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 280
                color: theme.surface
                border.color: theme.border
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: qsTr("文件")
                            color: theme.fgMuted
                            font.weight: Font.DemiBold
                            Layout.fillWidth: true
                        }

                        PLIconButton {
                            iconName: "folder-open"
                            tooltip: qsTr("刷新")
                            theme: editor.theme
                            onClicked: controller.refreshProjectFiles()
                        }
                    }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        ColumnLayout {
                            width: parent.width
                            spacing: 2

                            Repeater {
                                model: controller.projectFiles

                                delegate: Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 30
                                    radius: 6
                                    color: rowMouse.containsMouse || modelData.path === controller.currentFileRelativePath
                                           ? theme.surfaceAlt
                                           : "transparent"

                                    MouseArea {
                                        id: rowMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: modelData.editable ? Qt.PointingHandCursor : Qt.ArrowCursor
                                        onClicked: {
                                            if (modelData.editable) {
                                                controller.openProjectFile(modelData.path)
                                            }
                                        }
                                    }

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 8 + modelData.depth * 14
                                        anchors.rightMargin: 8
                                        spacing: 8

                                        Icon {
                                            name: modelData.icon
                                            size: 15
                                            color: modelData.path === controller.currentFileRelativePath
                                                   ? theme.accent
                                                   : theme.fgMuted
                                            Layout.alignment: Qt.AlignVCenter
                                        }

                                        Label {
                                            Layout.fillWidth: true
                                            text: modelData.name
                                            color: modelData.path === controller.currentFileRelativePath
                                                   ? theme.accent
                                                   : modelData.editable ? theme.fg : theme.fgMuted
                                            font.weight: modelData.path === controller.currentFileRelativePath
                                                         ? Font.DemiBold
                                                         : Font.Normal
                                            elide: Text.ElideRight
                                        }
                                    }
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                visible: controller.projectFiles.length === 0
                                text: qsTr("没有可显示的项目文件")
                                color: theme.fgMuted
                                horizontalAlignment: Text.AlignHCenter
                                topPadding: 20
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillHeight: true
                Layout.fillWidth: true
                color: theme.bg

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: (controller.editorDirty ? "● " : "") +
                                  (controller.currentFileRelativePath || qsTr("未选择文件"))
                            color: theme.fg
                            font.weight: Font.DemiBold
                            Layout.fillWidth: true
                            elide: Text.ElideMiddle
                        }

                        Label {
                            text: controller.editorStatus
                            color: theme.fgMuted
                            font.pixelSize: 12
                        }
                    }

                    PLCard {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        theme: editor.theme
                        padding: 0

                        TextArea {
                            id: sourceEditor
                            anchors.fill: parent
                            anchors.margins: 12
                            enabled: controller.currentFileRelativePath.length > 0
                            readOnly: false
                            wrapMode: controller.settings.wordWrap ? TextArea.Wrap : TextArea.NoWrap
                            color: theme.fg
                            placeholderText: qsTr("选择一个 .tex / .bib 文件开始编辑")
                            placeholderTextColor: theme.fgMuted
                            selectedTextColor: theme.fg
                            selectionColor: theme.accentSoft
                            font.family: controller.settings.editorFontFamily
                            font.pixelSize: controller.settings.editorFontSize
                            tabStopDistance: fontMetrics.averageCharacterWidth * controller.settings.tabWidth
                            background: null
                            onTextChanged: {
                                if (!editor.syncingEditorText) {
                                    controller.updateCurrentFileContent(text)
                                }
                            }

                            FontMetrics {
                                id: fontMetrics
                                font: sourceEditor.font
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 380
                color: theme.surface
                border.color: theme.border
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: qsTr("PDF / 日志")
                            color: theme.fgMuted
                            font.weight: Font.DemiBold
                            Layout.fillWidth: true
                        }

                        PLButton {
                            text: qsTr("打开 PDF")
                            iconName: "folder-open"
                            theme: editor.theme
                            enabled: controller.pdfPath.length > 0
                            Layout.preferredWidth: 116
                            onClicked: controller.openPdf()
                        }
                    }

                    PLCard {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 88
                        theme: editor.theme

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 6

                            Label {
                                Layout.fillWidth: true
                                text: controller.pdfPath.length > 0
                                      ? controller.pdfPath
                                      : qsTr("编译后在此显示 PDF 输出路径")
                                color: controller.pdfPath.length > 0 ? theme.fg : theme.fgMuted
                                elide: Text.ElideMiddle
                            }

                            Label {
                                Layout.fillWidth: true
                                text: controller.compiling ? qsTr("正在编译...") : controller.editorStatus
                                color: theme.fgMuted
                                font.pixelSize: 12
                                elide: Text.ElideRight
                            }
                        }
                    }

                    PLCard {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        theme: editor.theme
                        padding: 0

                        TextArea {
                            anchors.fill: parent
                            anchors.margins: 10
                            readOnly: true
                            wrapMode: TextArea.Wrap
                            color: theme.fg
                            selectedTextColor: theme.fg
                            selectionColor: theme.accentSoft
                            placeholderText: qsTr("编译日志")
                            placeholderTextColor: theme.fgMuted
                            text: controller.compileLog
                            font.family: controller.settings.editorFontFamily
                            font.pixelSize: Math.max(11, controller.settings.editorFontSize - 1)
                            background: null
                        }
                    }
                }
            }
        }
    }
}
