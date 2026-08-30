import QtQuick
import OmaGames

// The hold box or one slot of the next queue: a labelled frame with a
// tetromino drawn small inside it, in the same colour the well gives it.
OmaPanel {
    id: box

    property int piece: -1
    property var colors: []
    property real cell: 12
    // Dimmed while hold is spent, so the rule shows without a word.
    property bool available: true

    readonly property var shape: game.pieceShape(piece)

    padding: Math.round(cell * 0.5)
    implicitWidth: cell * 4 + 2 * padding
    implicitHeight: cell * 2 + 2 * padding
    color: theme.alpha(theme.foreground, 0.05)
    opacity: available ? 1 : 0.45

    Item {
        anchors.centerIn: parent
        width: box.shape.width * box.cell
        height: box.shape.height * box.cell

        Repeater {
            model: box.shape.cells

            Rectangle {
                required property var modelData
                x: modelData.x * box.cell
                y: modelData.y * box.cell
                width: box.cell
                height: box.cell
                color: box.piece >= 0 && box.piece < box.colors.length ? box.colors[box.piece] : "transparent"
                border.width: 1
                border.color: theme.alpha(theme.background, 0.45)
            }
        }
    }
}
