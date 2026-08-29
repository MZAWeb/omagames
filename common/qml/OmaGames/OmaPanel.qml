import QtQuick

// A slightly raised surface (dialogs, side panels, score boxes).
Rectangle {
    default property alias content: inner.data
    property real padding: 16 * theme.textScale

    radius: 12
    color: theme.mix(theme.background, theme.foreground, 0.06)
    border.width: 1
    border.color: theme.alpha(theme.foreground, 0.12)

    Item {
        id: inner
        anchors.fill: parent
        anchors.margins: parent.padding
    }
}
