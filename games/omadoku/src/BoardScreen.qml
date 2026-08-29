import QtQuick
import QtQuick.Layouts
import OmaGames
import Omadoku

// The playing screen: status line, board, digit pad and actions. All rules
// live in `game`; this only renders state and forwards intent.
FocusScope {
    id: root

    focus: true

    property var difficultyNames: [qsTr("Easy"), qsTr("Medium"), qsTr("Hard")]

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

    Keys.onPressed: function(event) {
        var digit = root.digitFor(event);
        if (digit > 0) {
            if (event.modifiers & Qt.ControlModifier)
                game.enterValue(digit);
            else if (event.modifiers & Qt.ShiftModifier)
                game.toggleNote(digit);
            else
                game.toggleHighlight(digit);
        } else if (event.key === Qt.Key_Left || event.key === Qt.Key_H) {
            game.moveSelection(0, -1);
        } else if (event.key === Qt.Key_Right || event.key === Qt.Key_L) {
            game.moveSelection(0, 1);
        } else if (event.key === Qt.Key_Up || event.key === Qt.Key_K) {
            game.moveSelection(-1, 0);
        } else if (event.key === Qt.Key_Down || event.key === Qt.Key_J) {
            game.moveSelection(1, 0);
        } else if (event.key === Qt.Key_N) {
            game.toggleNotesMode();
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
                text: root.difficultyNames[game.difficulty]
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

        SudokuBoardView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 200
        }

        DigitPad {
            Layout.fillWidth: true
            keyHeight: Math.max(32, Math.min(52 * theme.textScale, root.width / 9))
            onDigitPressed: function(digit) { game.pressPad(digit); }
        }

        DigitLegend { Layout.alignment: Qt.AlignHCenter }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8 * theme.textScale

            HintButton {
                Layout.fillWidth: true
                text: game.notesMode ? qsTr("Notes on") : qsTr("Notes")
                primary: game.notesMode
                keyHint: qsTr("N")
                onClicked: game.toggleNotesMode()
            }
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
            HintButton {
                Layout.fillWidth: true
                text: qsTr("New game")
                keyHint: qsTr("Esc")
                onClicked: root.leaveRequested()
            }
        }
    }
}
