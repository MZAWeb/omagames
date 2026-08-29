import QtQuick

// The last few table events, newest at the bottom and brightest.
Column {
    spacing: 2 * theme.textScale
    Repeater {
        model: game.log
        delegate: Text {
            required property string modelData
            required property int index
            text: modelData
            color: theme.foreground
            opacity: 0.35 + 0.65 * (index + 1) / game.log.length
            font.pixelSize: 12 * theme.textScale
            elide: Text.ElideRight
            width: parent.width
        }
    }
}
