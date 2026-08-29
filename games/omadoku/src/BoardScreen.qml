import QtQuick
import QtQuick.Layouts

// The playing screen: status line, board and, beside it, the keypad with its
// click mode and the remaining actions. All rules live in `game`; this only
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

    // Which digit a key press means. Layouts disagree loudly here: US sends
    // the symbol row with Shift (Key_Exclam and friends), AZERTY sends the
    // digits themselves with Shift, and anything else falls back to the
    // physical top-row scan codes (X11 keycodes 10-18).
    readonly property var shiftedRowKeys: [Qt.Key_Exclam, Qt.Key_At, Qt.Key_NumberSign,
        Qt.Key_Dollar, Qt.Key_Percent, Qt.Key_AsciiCircum, Qt.Key_Ampersand,
        Qt.Key_Asterisk, Qt.Key_ParenLeft]

    function digitFor(event) {
        if (event.key >= Qt.Key_1 && event.key <= Qt.Key_9)
            return event.key - Qt.Key_0;
        if (event.modifiers & Qt.ShiftModifier) {
            var symbol = "!@#$%^&*(".indexOf(event.text);
            if (symbol >= 0)
                return symbol + 1;
            var known = root.shiftedRowKeys.indexOf(event.key);
            if (known >= 0)
                return known + 1;
        }
        if (event.nativeScanCode >= 10 && event.nativeScanCode <= 18)
            return event.nativeScanCode - 9;
        return 0;
    }

    // A modifier names the action outright; without one the selected mode
    // decides. The bridge does the deciding, this only reports what was typed.
    function overrideFor(event) {
        if (event.modifiers & Qt.ControlModifier)
            return "fill";
        if (event.modifiers & Qt.ShiftModifier)
            return "note";
        if (event.modifiers & Qt.AltModifier)
            return "highlight";
        return "";
    }

    Keys.onPressed: function(event) {
        var digit = root.digitFor(event);
        if (digit > 0) {
            game.pressDigit(digit, root.overrideFor(event));
        } else if (event.key === Qt.Key_Left || event.key === Qt.Key_H) {
            game.moveSelection(0, -1);
        } else if (event.key === Qt.Key_Right || event.key === Qt.Key_L) {
            game.moveSelection(0, 1);
        } else if (event.key === Qt.Key_Up || event.key === Qt.Key_K) {
            game.moveSelection(-1, 0);
        } else if (event.key === Qt.Key_Down || event.key === Qt.Key_J) {
            game.moveSelection(1, 0);
        } else if (event.key === Qt.Key_N) {
            game.cyclePadMode();
        } else if (event.key === Qt.Key_V) {
            game.validateAsYouGo = !game.validateAsYouGo;
        } else if (event.key === Qt.Key_R) {
            game.restart();
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
            Item { Layout.fillWidth: true }
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

        Item {
            id: content
            Layout.fillWidth: true
            Layout.fillHeight: true

            // One cell of the board is the unit the keypad beside it is sized
            // from, so board and pad always scale together: 9 units of board,
            // half a unit of gap and roughly four units of side column.
            readonly property real unit: Math.floor(Math.min(width / 13.6, height / 9))

            SudokuBoardView {
                id: boardView
                width: content.unit * 9
                height: width
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
            }

            ColumnLayout {
                id: side
                anchors.left: boardView.right
                anchors.leftMargin: content.unit * 0.5
                anchors.right: parent.right
                anchors.top: boardView.top
                anchors.bottom: boardView.bottom
                spacing: 10 * theme.textScale

                // Room left for the keypad once the mode selector has taken
                // its share. A key plus its gap is 1.08 keys wide, hence the
                // 3.24: expressing it that way keeps the keypad's own spacing
                // out of the binding and avoids a loop.
                readonly property real keyBudget: height - modes.implicitHeight - 2 * spacing

                DigitPad {
                    Layout.alignment: Qt.AlignHCenter
                    keySize: Math.max(18, Math.min(content.unit * 1.2,
                                                   side.keyBudget / 3.24,
                                                   side.width / 3.24))
                    onDigitPressed: function(digit) { game.pressDigit(digit); }
                }

                PadModeSelector {
                    id: modes
                    Layout.fillWidth: true
                }

                Item { Layout.fillHeight: true }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8 * theme.textScale

            HintButton {
                Layout.fillWidth: true
                text: qsTr("Erase")
                keyHint: qsTr("Bksp")
                onClicked: game.erase()
            }
            HintButton {
                Layout.fillWidth: true
                text: qsTr("Undo")
                keyHint: qsTr("Ctrl+Z")
                actionEnabled: game.canUndo
                onClicked: game.undo()
            }
            HintButton {
                Layout.fillWidth: true
                text: qsTr("Restart")
                keyHint: qsTr("R")
                onClicked: game.restart()
            }
            // A toggle drawn as a button: filled while on, outlined while off,
            // so its state reads at a glance among the plain actions.
            HintButton {
                Layout.fillWidth: true
                text: qsTr("Validate")
                keyHint: qsTr("V")
                primary: game.validateAsYouGo
                onClicked: game.validateAsYouGo = !game.validateAsYouGo
            }
            HintButton {
                Layout.fillWidth: true
                text: qsTr("New game")
                keyHint: qsTr("Esc")
                onClicked: root.leaveRequested()
            }
        }
    }
}
