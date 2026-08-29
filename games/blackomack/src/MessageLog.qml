import QtQuick

// The last few table events, newest at the bottom and brightest.
Column {
    id: logView
    readonly property var visibleLog: game.log.length <= 3 ? game.log : game.log.slice(game.log.length - 3)
    spacing: 2 * theme.textScale
    Repeater {
        model: logView.visibleLog
        delegate: Text {
            required property string modelData
            required property int index
            text: modelData
            color: theme.foreground
            opacity: 0.7 + 0.3 * (index + 1) / logView.visibleLog.length
            font.pixelSize: 12 * theme.textScale
            elide: Text.ElideRight
            width: parent.width
        }
    }
}
