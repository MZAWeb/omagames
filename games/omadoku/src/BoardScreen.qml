import QtQuick
import QtQuick.Layouts

// The playing screen: a compact header, the square board as the hero, and the
// rail of controls beside it — or under it, when the window is shaped so that
// the board comes out bigger that way. All rules live in `game`; this only
// renders state and forwards intent.
FocusScope {
    id: root

    focus: true

    signal leaveRequested()

    function formatTime(seconds) {
        var minutes = Math.floor(seconds / 60);
        var rest = seconds % 60;
        return minutes + ":" + (rest < 10 ? "0" : "") + rest;
    }

    // Shift turns a move into a sweep: the cursor goes the same way and every
    // cell it crosses joins the selection.
    function moveBy(event, deltaRow, deltaColumn) {
        if (event.modifiers & Qt.ShiftModifier)
            game.extendSelection(deltaRow, deltaColumn);
        else
            game.moveCursor(deltaRow, deltaColumn);
    }

    Keys.onPressed: function(event) {
        var digit = game.digitForKey(event.key, event.modifiers, event.text);
        if (digit > 0) {
            game.pressDigitKey(digit, event.modifiers);
        } else if (event.key === Qt.Key_Left || event.key === Qt.Key_H) {
            root.moveBy(event, 0, -1);
        } else if (event.key === Qt.Key_Right || event.key === Qt.Key_L) {
            root.moveBy(event, 0, 1);
        } else if (event.key === Qt.Key_Up || event.key === Qt.Key_K) {
            root.moveBy(event, -1, 0);
        } else if (event.key === Qt.Key_Down || event.key === Qt.Key_J) {
            root.moveBy(event, 1, 0);
        } else if (event.key === Qt.Key_N) {
            game.cycleClickMode();
        } else if (event.key === Qt.Key_V) {
            game.validateAsYouGo = !game.validateAsYouGo;
        } else if (event.key === Qt.Key_A) {
            game.autoNotes = !game.autoNotes;
        } else if (event.key === Qt.Key_R) {
            game.requestRestart();
        } else if (event.key === Qt.Key_Backspace || event.key === Qt.Key_Delete
                   || event.key === Qt.Key_0) {
            game.erase();
        } else {
            return;
        }
        event.accepted = true;
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14 * theme.textScale
        spacing: 10 * theme.textScale

        RowLayout {
            Layout.fillWidth: true

            Text {
                text: game.difficultyLabel
                color: theme.foreground
                font.pixelSize: 17 * theme.textScale
                font.bold: true
            }
            Text {
                Layout.leftMargin: 10 * theme.textScale
                Layout.fillWidth: true
                text: qsTr("needs %1").arg(game.techniqueLabel)
                color: theme.mix(theme.background, theme.foreground, 0.6)
                font.pixelSize: 14 * theme.textScale
                elide: Text.ElideRight
            }
            Text {
                text: game.filledCount + "/81"
                color: theme.mix(theme.background, theme.foreground, 0.6)
                font.pixelSize: 14 * theme.textScale
            }
            Text {
                Layout.leftMargin: 12 * theme.textScale
                text: root.formatTime(game.elapsedSeconds)
                color: theme.mix(theme.background, theme.foreground, 0.6)
                font.pixelSize: 14 * theme.textScale
            }
        }

        // Board and controls. Both shapes give the board every pixel the
        // controls do not need, so the layout is chosen by which one leaves it
        // bigger — which is what makes a narrow window, or a large text scale,
        // fold the rail underneath on its own.
        Item {
            id: content

            Layout.fillWidth: true
            Layout.fillHeight: true

            readonly property real gap: 14 * theme.textScale
            readonly property real railWidth:
                Math.max(280 * theme.textScale, Math.min(340 * theme.textScale, width * 0.34))
            // The rail needs this much height for its keypad to stay usable.
            readonly property real railMinHeight: 360 * theme.textScale
            readonly property bool tightSheet: width < 720 * theme.textScale
            readonly property real sheetHeight: (tightSheet ? 232 : 186) * theme.textScale

            readonly property real railBoard: Math.min(width - railWidth - gap, height)
            readonly property real sheetBoard: Math.min(width, height - sheetHeight - gap)
            readonly property bool compact: railBoard < railMinHeight || sheetBoard > railBoard

            Loader {
                anchors.fill: parent
                sourceComponent: content.compact ? sheetLayout : railLayout
            }

            Component {
                id: railLayout

                Item {
                    readonly property real boardSize: Math.max(0, content.railBoard)

                    SudokuBoardView {
                        id: railBoardView
                        width: parent.boardSize
                        height: width
                        x: Math.round((parent.width - width - content.gap - content.railWidth) / 2)
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    BoardRail {
                        anchors.left: railBoardView.right
                        anchors.leftMargin: content.gap
                        anchors.top: railBoardView.top
                        anchors.bottom: railBoardView.bottom
                        width: content.railWidth
                        onLeaveRequested: root.leaveRequested()
                    }
                }
            }

            Component {
                id: sheetLayout

                Item {
                    SudokuBoardView {
                        width: Math.max(0, content.sheetBoard)
                        height: width
                        anchors.horizontalCenter: parent.horizontalCenter
                        y: Math.round((parent.height - content.sheetHeight - content.gap - height) / 2)
                    }

                    BoardSheet {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: content.sheetHeight
                        tight: content.tightSheet
                        onLeaveRequested: root.leaveRequested()
                    }
                }
            }
        }
    }
}
