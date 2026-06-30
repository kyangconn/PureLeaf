import QtQuick
import QtQuick.Controls

ComboBox {
    id: root

    property var theme: controller.theme

    implicitHeight: 36
    leftPadding: 12
    rightPadding: 36

    contentItem: Text {
        leftPadding: 0
        rightPadding: 0
        text: root.displayText
        color: theme.fg
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    indicator: Icon {
        x: root.width - width - 10
        y: (root.height - height) / 2
        name: "chevron-down"
        size: 16
        color: theme.fgMuted
        hovered: root.hovered
        hoverColor: theme.fg
    }

    background: Rectangle {
        radius: theme.radius
        color: root.activeFocus ? theme.surface : theme.surfaceAlt
        border.color: root.activeFocus ? theme.accent : theme.border
        border.width: 1
    }

    delegate: ItemDelegate {
        width: root.width
        height: 34
        highlighted: root.highlightedIndex === index

        contentItem: Text {
            text: modelData
            color: highlighted ? root.theme.accent : root.theme.fg
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            color: highlighted ? root.theme.accentSoft : root.theme.surface
        }
    }

    popup: Popup {
        y: root.height + 4
        width: root.width
        // implicitHeight is the *whole* popup (incl. padding). The previous
        // version used contentItem.implicitHeight directly, so padding ate
        // into the content area (~2.7 rows). Combined with
        // highlightFollowsCurrentItem scrolling to the selected last item,
        // the top row ("亮色") was clipped out of view. Add padding back so
        // the content area equals the full list height.
        implicitHeight: Math.min(contentItem.implicitHeight + topPadding + bottomPadding, 260)
        padding: 4

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: root.popup.visible ? root.delegateModel : null
            currentIndex: root.highlightedIndex
        }

        background: Rectangle {
            color: root.theme.surface
            radius: root.theme.radius
            border.color: root.theme.border
            border.width: 1
        }
    }
}
