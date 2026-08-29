import QtQuick

// The 9x9 grid: cells, thin cell separators and thick 3x3 box separators.
Item {
    id: view

    readonly property real cellSize: Math.floor(Math.min(width, height) / 9)
    readonly property real boardSize: cellSize * 9
    readonly property real thin: Math.max(1, Math.round(cellSize * 0.02))
    readonly property real thick: Math.max(2, Math.round(cellSize * 0.06))

    Item {
        id: board
        width: view.boardSize
        height: view.boardSize
        anchors.centerIn: parent

        Rectangle {
            anchors.fill: parent
            color: theme.mix(theme.background, theme.foreground, 0.03)
            radius: view.thick
        }

        Repeater {
            model: game.cells

            SudokuCell {
                readonly property int cellIndex: model.index
                readonly property int cellRow: Math.floor(cellIndex / 9)
                readonly property int cellColumn: cellIndex % 9

                x: cellColumn * view.cellSize
                y: cellRow * view.cellSize
                width: view.cellSize
                height: view.cellSize
                cellSize: view.cellSize

                value: model.value
                notes: model.notes
                given: model.given
                wrong: model.wrong
                selected: game.selectedIndex === cellIndex
                peer: game.selectedIndex >= 0 && !selected
                      && (Math.floor(game.selectedIndex / 9) === cellRow
                          || game.selectedIndex % 9 === cellColumn
                          || (Math.floor(Math.floor(game.selectedIndex / 9) / 3) === Math.floor(cellRow / 3)
                              && Math.floor((game.selectedIndex % 9) / 3) === Math.floor(cellColumn / 3)))
                highlighted: !selected && model.value > 0 && model.value === game.highlightDigit
                sameDigit: !selected && !highlighted && model.value > 0 && model.value === game.selectedValue
                onClicked: game.select(cellIndex)
            }
        }

        // Separators: every line is drawn on top of the cells so the tint of a
        // selected cell never eats the grid.
        Repeater {
            model: 10
            Rectangle {
                required property int index
                readonly property bool major: index % 3 === 0
                width: major ? view.thick : view.thin
                height: board.height
                x: Math.min(index * view.cellSize - width / 2, board.width - width)
                color: theme.alpha(theme.foreground, major ? 0.55 : 0.18)
            }
        }
        Repeater {
            model: 10
            Rectangle {
                required property int index
                readonly property bool major: index % 3 === 0
                height: major ? view.thick : view.thin
                width: board.width
                y: Math.min(index * view.cellSize - height / 2, board.height - height)
                color: theme.alpha(theme.foreground, major ? 0.55 : 0.18)
            }
        }
    }
}
