import QtQuick

QtObject {
    id: root

    property var source: ({})

    readonly property bool dark: source.dark === true
    readonly property color bg: source.bg || "#fafafa"
    readonly property color surface: source.surface || "#ffffff"
    readonly property color surfaceAlt: source.surfaceAlt || "#f2f4f7"
    readonly property color fg: source.fg || "#1f1f1f"
    readonly property color fgMuted: source.fgMuted || "#667085"
    readonly property color border: source.border || "#d0d5dd"
    readonly property color accent: source.accent || "#2e7d32"
    readonly property color accentSoft: source.accentSoft || "#e7f4e8"
    readonly property color danger: source.danger || "#d92d20"
    readonly property int radius: source.radius || 10
    readonly property int spacing: source.spacing || 16
    readonly property int titleBarHeight: source.titleBarHeight || 44
}
