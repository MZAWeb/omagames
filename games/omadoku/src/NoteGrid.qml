import QtQuick

// The nine pencil-mark slots of a cell, drawn from a 9-bit mask. Each digit
// keeps its own slot whether or not it is set, so a mark never moves when its
// neighbours come and go — that stability is what makes notes scannable.
Grid {
    id: notes

    property int mask: 0
    property real cellSize: width

    columns: 3
    rows: 3

    Repeater {
        model: 9

        Text {
            required property int index

            width: notes.width / 3
            height: notes.height / 3
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: (notes.mask & (1 << index)) ? (index + 1).toString() : ""
            color: theme.mix(theme.background, theme.foreground, 0.62)
            font.pixelSize: Math.max(1, Math.round(notes.cellSize * 0.26))
        }
    }
}
