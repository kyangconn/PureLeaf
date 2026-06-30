import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: settingsPage

    property var theme: controller.theme

    QtObject {
        id: form

        property string compilerType: controller.settings.compilerType
        property string compilerPath: controller.settings.compilerPath
        property string editorFontFamily: controller.settings.editorFontFamily
        property int editorFontSize: controller.settings.editorFontSize
        property int tabWidth: controller.settings.tabWidth
        property bool insertSpaces: controller.settings.insertSpaces
        property bool autoSaveEnabled: controller.settings.autoSaveEnabled
        property string appTheme: controller.settings.appTheme
        property string controlsStyle: controller.settings.controlsStyle

        function syncFromController() {
            compilerType = controller.settings.compilerType
            compilerPath = controller.settings.compilerPath
            editorFontFamily = controller.settings.editorFontFamily
            editorFontSize = controller.settings.editorFontSize
            tabWidth = controller.settings.tabWidth
            insertSpaces = controller.settings.insertSpaces
            autoSaveEnabled = controller.settings.autoSaveEnabled
            appTheme = controller.settings.appTheme
            controlsStyle = controller.settings.controlsStyle
        }

        function apply() {
            controller.applySettings({
                "compilerType": compilerType,
                "compilerPath": compilerPath,
                "editorFontFamily": editorFontFamily,
                "editorFontSize": editorFontSize,
                "tabWidth": tabWidth,
                "insertSpaces": insertSpaces,
                "autoSaveEnabled": autoSaveEnabled,
                "appTheme": appTheme,
                "controlsStyle": controlsStyle
            })
        }
    }

    Connections {
        target: controller
        function onSettingsChanged() {
            form.syncFromController()
        }
    }

    Component.onCompleted: form.syncFromController()

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
                    theme: settingsPage.theme
                    onClicked: controller.backHome()
                }

                Label {
                    text: qsTr("设置")
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    color: theme.fg
                    Layout.fillWidth: true
                }

                PLButton {
                    text: qsTr("取消")
                    variant: "ghost"
                    theme: settingsPage.theme
                    Layout.preferredWidth: 82
                    onClicked: {
                        form.syncFromController()
                        controller.backHome()
                    }
                }

                PLButton {
                    text: qsTr("保存")
                    iconName: "copy"
                    variant: "primary"
                    theme: settingsPage.theme
                    Layout.preferredWidth: 92
                    onClicked: form.apply()
                }
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            clip: true

            ColumnLayout {
                width: parent.width
                spacing: 18

                SettingsSection {
                    title: qsTr("编译")

                    SettingField {
                        label: qsTr("编译器类型")
                        PLComboBox {
                            property var values: ["pdflatex", "xelatex", "lualatex"]
                            model: values
                            currentIndex: Math.max(0, values.indexOf(form.compilerType))
                            theme: settingsPage.theme
                            Layout.fillWidth: true
                            onActivated: form.compilerType = values[index]
                        }
                    }

                    SettingField {
                        label: qsTr("编译器路径")
                        PLTextField {
                            text: form.compilerPath
                            placeholderText: qsTr("从 PATH 自动检测")
                            theme: settingsPage.theme
                            Layout.fillWidth: true
                            onTextChanged: form.compilerPath = text
                        }
                    }
                }

                SettingsSection {
                    title: qsTr("编辑器")

                    SettingField {
                        label: qsTr("字体")
                        PLComboBox {
                            model: Qt.fontFamilies()
                            currentIndex: Math.max(0, model.indexOf(form.editorFontFamily))
                            theme: settingsPage.theme
                            Layout.fillWidth: true
                            onActivated: form.editorFontFamily = model[index]
                        }
                    }

                    SettingField {
                        label: qsTr("字号")
                        PLSpinBox {
                            from: 8
                            to: 36
                            value: form.editorFontSize
                            theme: settingsPage.theme
                            onValueModified: form.editorFontSize = value
                        }
                    }

                    SettingField {
                        label: qsTr("Tab 宽度")
                        PLSpinBox {
                            from: 2
                            to: 8
                            stepSize: 2
                            value: form.tabWidth
                            theme: settingsPage.theme
                            onValueModified: form.tabWidth = value
                        }
                    }

                    SettingField {
                        label: qsTr("缩进空格")
                        CheckBox {
                            checked: form.insertSpaces
                            text: qsTr("使用空格代替 Tab")
                            onToggled: form.insertSpaces = checked
                        }
                    }

                    SettingField {
                        label: qsTr("自动保存")
                        CheckBox {
                            checked: form.autoSaveEnabled
                            text: qsTr("启用")
                            onToggled: form.autoSaveEnabled = checked
                        }
                    }
                }

                SettingsSection {
                    title: qsTr("外观")

                    SettingField {
                        label: qsTr("主题")
                        PLComboBox {
                            property var values: ["light", "dark", "system"]
                            model: [qsTr("亮色"), qsTr("暗色"), qsTr("跟随系统")]
                            currentIndex: Math.max(0, values.indexOf(form.appTheme))
                            theme: settingsPage.theme
                            Layout.fillWidth: true
                            onActivated: {
                                form.appTheme = values[index]
                                controller.applySettings({ "appTheme": values[index] })
                            }
                        }
                    }

                    SettingField {
                        label: qsTr("控件风格")
                        PLComboBox {
                            property var values: ["platform", "fluent", "fusion", "breeze"]
                            model: [qsTr("平台默认"), "Fluent WinUI 3", "Fusion", "Breeze"]
                            currentIndex: Math.max(0, values.indexOf(form.controlsStyle))
                            theme: settingsPage.theme
                            Layout.fillWidth: true
                            onActivated: form.controlsStyle = values[index]
                        }
                    }
                }

                Item {
                    Layout.preferredHeight: 32
                }
            }
        }
    }

    component SettingsSection : PLCard {
        property string title
        default property alias fields: fields.data

        Layout.fillWidth: true
        Layout.leftMargin: 28
        Layout.rightMargin: 28
        Layout.topMargin: 18
        Layout.preferredHeight: sectionColumn.implicitHeight + padding * 2
        theme: settingsPage.theme

        ColumnLayout {
            id: sectionColumn
            anchors.fill: parent
            spacing: 14

            Label {
                text: title
                color: settingsPage.theme.fg
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }

            GridLayout {
                id: fields
                Layout.fillWidth: true
                columns: settingsPage.width >= 820 ? 2 : 1
                columnSpacing: 22
                rowSpacing: 14
            }
        }
    }

    component SettingField : ColumnLayout {
        property string label
        default property alias fieldContent: fieldSlot.data

        Layout.fillWidth: true
        spacing: 6

        Label {
            text: parent.label
            color: settingsPage.theme.fgMuted
            font.pixelSize: 12
        }

        ColumnLayout {
            id: fieldSlot
            Layout.fillWidth: true
            spacing: 0
        }
    }
}
