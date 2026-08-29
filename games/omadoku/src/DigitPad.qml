import QtQuick

// The digits as a phone-style 3x3 keypad beside the board. A digit that is
// fully placed dims itself; the highlighted one stays lit.
Grid {
    id: pad

    property real keySize: 44 * theme.textScale
    // Matches the board's tint timing, so pad and cells settle together.
    readonly property int tintMs: 90
    signal digitPressed(int digit)

    columns: 3
    rows: 3
    spacing: Math.round(keySize * 0.12)

    Repeater {
        model: 9

        Rectangle {
            required property int index
            readonly property int digit: index + 1
            readonly property bool done: game.digitCounts[index] >= 9
            readonly property bool highlighted: game.highlightDigit === digit

            width: pad.keySize
            height: pad.keySize
            radius: Math.round(height * 0.18)
            color: mouse.pressed ? theme.alpha(theme.accent, 0.35)
                 : highlighted ? theme.alpha(theme.accent, 0.28)
                 : mouse.containsMouse ? theme.alpha(theme.foreground, 0.12)
                 : theme.alpha(theme.foreground, 0.06)
            border.width: 1
            border.color: highlighted ? theme.alpha(theme.accent, 0.7)
                        : theme.alpha(theme.foreground, done ? 0.08 : 0.22)
            opacity: done ? 0.45 : 1.0
            Behavior on color { ColorAnimation { duration: pad.tintMs } }

            Text {
                anchors.centerIn: parent
                text: parent.digit.toString()
                color: theme.foreground
                font.pixelSize: Math.round(pad.keySize * 0.42)
                font.bold: true
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
