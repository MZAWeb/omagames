import QtQuick
import QtQuick.Layouts
// FieldView is the C++ item main.cpp registers under `Omasweeper`; there is
// no qmldir for qmllint to find it, so the import and the one block that uses
// it are excused from the lint that needs one.
import Omasweeper // qmllint disable import

// The playing screen: header, the painted minefield at a whole number of
// pixels per cell, and the key legend. All rules live in `game`; this only
// renders state and forwards keys.
FocusScope {
    id: root

    // Below this a number stops being readable, so the board keeps its size
    // and the window refuses to shrink further instead.
    readonly property int minimumCell: 8

    focus: true

    signal leaveRequested()

    Keys.onPressed: function(event) {
        switch (event.key) {
        case Qt.Key_Up: case Qt.Key_K: game.moveCursor(0, -1); break;
        case Qt.Key_Down: case Qt.Key_J: game.moveCursor(0, 1); break;
        case Qt.Key_Left: case Qt.Key_H: game.moveCursor(-1, 0); break;
        case Qt.Key_Right: case Qt.Key_L: game.moveCursor(1, 0); break;
        case Qt.Key_Space: case Qt.Key_Return: case Qt.Key_Enter: game.revealAtCursor(); break;
        case Qt.Key_F: game.flagAtCursor(); break;
        case Qt.Key_R: game.restart(); break;
        case Qt.Key_Escape: root.leaveRequested(); break;
        default: return;
        }
        event.accepted = true;
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14 * theme.textScale
        spacing: 10 * theme.textScale

        PlayHeader { Layout.fillWidth: true }

        Item {
            id: arena
            Layout.fillWidth: true
            Layout.fillHeight: true

            readonly property int cell: Math.max(root.minimumCell,
                Math.floor(Math.min(width / game.width, height / game.height)))

            Item {
                id: view
                anchors.centerIn: parent
                width: arena.cell * game.width
                height: arena.cell * game.height

                // qmllint disable import unresolved-type missing-type incompatible-type missing-property
                FieldView {
                    anchors.fill: parent
                    cellSize: arena.cell
                    source: game
                    hiddenColor: theme.mix(theme.background, theme.foreground, 0.16)
                    hiddenHighlightColor: theme.alpha(theme.lightForeground, 0.18)
                    hiddenShadowColor: theme.alpha(theme.darkBackground, 0.45)
                    revealedColor: theme.mix(theme.background, theme.darkerBackground, 0.7)
                    gridColor: theme.alpha(theme.foreground, 0.08)
                    cursorColor: theme.accent
                    flagColor: theme.yellow
                    mineColor: theme.foreground
                    explodedColor: theme.alpha(theme.red, 0.7)
                    wrongFlagColor: theme.red
                    number1Color: theme.blue
                    number2Color: theme.green
                    number3Color: theme.red
                    number4Color: theme.magenta
                    number5Color: theme.orange
                    number6Color: theme.cyan
                    number7Color: theme.foreground
                    number8Color: theme.muted
                }
                // qmllint enable import unresolved-type missing-type incompatible-type missing-property
            }

            // The board's own frame, so it reads as one object.
            Rectangle {
                anchors.fill: view
                anchors.margins: -2
                radius: 4
                color: "transparent"
                border.width: 2
                border.color: theme.alpha(theme.accent, 0.5)
            }
        }

        KeyLegend {
            Layout.fillWidth: true
            compact: true
        }
    }
}
