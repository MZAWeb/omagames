import QtQuick
import QtQuick.Layouts
import OmaGames
// FieldView is the C++ item main.cpp registers under `Omatris`; there is no
// qmldir for qmllint to find it, so the import and the one block that uses
// it are excused from the lint that needs one.
import Omatris // qmllint disable import

// The playing screen: header, the hold box, the painted well, the next queue
// and the key legend. All rules live in `game`; this only renders state and
// forwards keys.
FocusScope {
    id: root

    focus: true

    signal leaveRequested()

    // The one place the tetromino palette is written down: the well and the
    // little boxes read the same array, in the engine's piece order.
    readonly property var pieceColors: [theme.cyan, theme.blue, theme.orange,
                                        theme.yellow, theme.green, theme.magenta, theme.red]

    Keys.onPressed: function(event) {
        switch (event.key) {
        case Qt.Key_Left:
            // Auto-repeat is the keyboard's; delayed auto shift is the game's.
            if (!event.isAutoRepeat) game.pressLeft();
            break;
        case Qt.Key_Right:
            if (!event.isAutoRepeat) game.pressRight();
            break;
        case Qt.Key_Down: game.setSoftDrop(true); break;
        case Qt.Key_Space: game.hardDrop(); break;
        case Qt.Key_Up: case Qt.Key_X: game.rotateCw(); break;
        case Qt.Key_Z: game.rotateCcw(); break;
        case Qt.Key_C: game.swapHold(); break;
        case Qt.Key_G: game.toggleGhost(); break;
        case Qt.Key_P: game.togglePause(); break;
        case Qt.Key_R: game.restart(); break;
        case Qt.Key_Escape: root.leaveRequested(); break;
        default: return;
        }
        event.accepted = true;
    }
    Keys.onReleased: function(event) {
        // Auto-repeat releases are not the finger leaving the key.
        if (event.isAutoRepeat)
            return;
        switch (event.key) {
        case Qt.Key_Left: game.releaseLeft(); break;
        case Qt.Key_Right: game.releaseRight(); break;
        case Qt.Key_Down: game.setSoftDrop(false); break;
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

            // Ten columns of well between two four-column rails, with half a
            // cell of air on each side of it.
            readonly property int cell: Math.max(6, Math.floor(Math.min(width / 20, height / game.boardHeight)))
            readonly property int rail: arena.cell * 4

            Row {
                anchors.centerIn: parent
                spacing: Math.round(arena.cell * 0.5)

                ColumnLayout {
                    width: arena.rail
                    spacing: 5 * theme.textScale

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 5 * theme.textScale
                        Text {
                            text: qsTr("Hold")
                            color: theme.mix(theme.background, theme.foreground, 0.6)
                            font.pixelSize: 12 * theme.textScale
                        }
                        Item { Layout.fillWidth: true }
                        OmaKeyHint { key: qsTr("C"); active: game.holdAvailable }
                    }
                    PieceBox {
                        Layout.fillWidth: true
                        piece: game.holdPiece
                        colors: root.pieceColors
                        cell: Math.round(arena.cell * 0.7)
                        available: game.holdAvailable
                    }
                    // Only worth saying when the ghost is gone: on, it shows.
                    Text {
                        Layout.fillWidth: true
                        Layout.topMargin: 4 * theme.textScale
                        visible: !game.ghostEnabled
                        text: qsTr("Ghost off")
                        color: theme.mix(theme.background, theme.foreground, 0.45)
                        font.pixelSize: 11 * theme.textScale
                    }
                    Item { Layout.fillHeight: true }
                }

                Item {
                    id: well
                    width: arena.cell * game.boardWidth
                    height: arena.cell * game.boardHeight

                    // qmllint disable import unresolved-type missing-type incompatible-type missing-property
                    FieldView {
                        anchors.fill: parent
                        cellSize: arena.cell
                        showGhost: game.ghostEnabled
                        source: game
                        pieceColors: root.pieceColors
                        emptyColor: theme.mix(theme.background, theme.darkerBackground, 0.7)
                        gridColor: theme.alpha(theme.foreground, 0.07)
                        ghostColor: theme.alpha(theme.foreground, 0.55)
                        flashColor: theme.brightForeground
                        lightColor: theme.brightForeground
                        shadeColor: theme.darkerBackground
                    }
                    // qmllint enable import unresolved-type missing-type incompatible-type missing-property

                    // The well's own frame, so it reads as one object.
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: -2
                        radius: 4
                        color: "transparent"
                        border.width: 2
                        border.color: theme.alpha(theme.accent, 0.5)
                    }

                    Item { id: popups; anchors.fill: parent }
                }

                ColumnLayout {
                    width: arena.rail
                    spacing: 5 * theme.textScale

                    Text {
                        text: qsTr("Next")
                        color: theme.mix(theme.background, theme.foreground, 0.6)
                        font.pixelSize: 12 * theme.textScale
                    }
                    Repeater {
                        model: game.nextQueue

                        PieceBox {
                            required property var modelData
                            Layout.fillWidth: true
                            piece: modelData
                            colors: root.pieceColors
                            cell: Math.round(arena.cell * 0.7)
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
            }

            Component {
                id: popupComponent
                BonusPopup { onFinished: destroy() }
            }

            Connections {
                target: game
                // One placement can be a T-spin, back-to-back and part of a
                // combo at once; each new label sits above the ones showing.
                function onBonusEarned(text, x, y) {
                    popupComponent.createObject(popups, {
                        text: text, anchorX: (x + 0.5) * arena.cell, anchorY: y * arena.cell,
                        stackIndex: popups.children.length });
                }
            }
        }

        KeyLegend {
            Layout.fillWidth: true
            compact: true
        }
    }
}
