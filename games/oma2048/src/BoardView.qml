import QtQuick

// The 4×4 well: a centred square field that scales with the window, the
// empty cells behind, and the tiles sliding across them. Tiles come from
// `game.tiles`, whose rows keep their identity by tile id, so a delegate
// survives a move and its x/y Behaviors carry it to the new cell.
Item {
    id: board

    readonly property int gridSize: game.boardSize
    readonly property real side: Math.min(width, height)
    readonly property real gap: side * 0.025
    readonly property real cell: (side - (gridSize + 1) * gap) / gridSize

    Rectangle {
        id: field
        anchors.centerIn: parent
        width: board.side
        height: board.side
        radius: board.gap
        color: theme.mix(theme.background, theme.darkerBackground, 0.6)
        border.width: 2
        border.color: theme.alpha(theme.accent, 0.5)

        Repeater {
            model: board.gridSize * board.gridSize

            Rectangle {
                required property int index
                x: board.gap + (index % board.gridSize) * (board.cell + board.gap)
                y: board.gap + Math.floor(index / board.gridSize) * (board.cell + board.gap)
                width: board.cell
                height: board.cell
                radius: board.cell * 0.08
                color: theme.alpha(theme.foreground, 0.05)
            }
        }

        Repeater {
            model: game.tiles

            TileDelegate {
                cell: board.cell
                gap: board.gap
            }
        }
    }
}
