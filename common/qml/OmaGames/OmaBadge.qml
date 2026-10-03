import QtQuick

// A word in a tinted lozenge for a state worth a glance: a streak kept
// going, a replay's speed, "Paused". `tint` colours the text, the border
// and, faintly, the fill.
Rectangle {
    id: badge

    property string text
    property color tint: theme.accent

    implicitWidth: label.implicitWidth + 12 * theme.textScale
    implicitHeight: label.implicitHeight + 5 * theme.textScale
    radius: 4
    color: theme.alpha(badge.tint, 0.18)
    border.width: 1
    border.color: theme.alpha(badge.tint, 0.5)

    Text {
        id: label
        anchors.centerIn: parent
        text: badge.text
        color: badge.tint
        font.pixelSize: 11 * theme.textScale
        font.bold: true
    }
}
