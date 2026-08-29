import QtQuick

// 1-9 in one row. A digit that is fully placed dims itself so the player can
// see at a glance what is left to do.
Row {
    id: pad

    property real keyHeight: 44 * theme.textScale
    signal digitPressed(int digit)

    spacing: Math.round(keyHeight * 0.12)

    Repeater {
        model: 9

        Rectangle {
            required property int index
            readonly property int digit: index + 1
            readonly property bool done: game.digitCounts[index] >= 9
            readonly property bool highlighted: game.highlightDigit === digit

            width: (pad.width - pad.spacing * 8) / 9
            height: pad.keyHeight
            radius: Math.round(height * 0.22)
            color: mouse.pressed ? theme.alpha(theme.accent, 0.35)
                 : highlighted ? theme.alpha(theme.accent, 0.28)
                 : mouse.containsMouse ? theme.alpha(theme.foreground, 0.12)
                 : theme.alpha(theme.foreground, 0.06)
            border.width: 1
            border.color: highlighted ? theme.alpha(theme.accent, 0.7)
                        : theme.alpha(theme.foreground, done ? 0.08 : 0.22)
            opacity: done ? 0.45 : 1.0
            Behavior on color { ColorAnimation { duration: 90 } }

            Text {
                anchors.centerIn: parent
                text: parent.digit.toString()
                color: game.notesMode ? theme.mix(theme.background, theme.foreground, 0.7) : theme.foreground
                font.pixelSize: Math.round(pad.keyHeight * 0.46)
                font.bold: !game.notesMode
            }

            MouseArea {
                id: mouse
                anchors.fill: parent
                hoverEnabled: true
                onClicked: pad.digitPressed(parent.digit)
            }
        }
    }
}
