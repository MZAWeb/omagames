import QtQuick
import QtQuick.Layouts
import OmaGames
import Omadoku

// The playing screen: status line, board, digit pad and actions. All rules
// live in `game`; this only renders and forwards intent.
FocusScope {
    id: root

    focus: true

    signal leaveRequested()

    function formatTime(seconds) {
        var minutes = Math.floor(seconds / 60);
        var rest = seconds % 60;
        return minutes + ":" + (rest < 10 ? "0" : "") + rest;
    }

    readonly property var difficultyNames: [qsTr("Easy"), qsTr("Medium"), qsTr("Hard")]

    Keys.onPressed: function(event) {
        if (event.key >= Qt.Key_1 && event.key <= Qt.Key_9)
            game.enterDigit(event.key - Qt.Key_0);
        else if (event.key === Qt.Key_Left || event.key === Qt.Key_H)
            game.moveSelection(0, -1);
        else if (event.key === Qt.Key_Right || event.key === Qt.Key_L)
            game.moveSelection(0, 1);
        else if (event.key === Qt.Key_Up || event.key === Qt.Key_K)
            game.moveSelection(-1, 0);
        else if (event.key === Qt.Key_Down || event.key === Qt.Key_J)
            game.moveSelection(1, 0);
        else if (event.key === Qt.Key_N)
            game.toggleNotesMode();
        else if (event.key === Qt.Key_Backspace || event.key === Qt.Key_Delete || event.key === Qt.Key_0)
            game.erase();
        else
            return;
        event.accepted = true;
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14 * theme.textScale
        spacing: 12 * theme.textScale

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
            onDigitPressed: function(digit) { game.enterDigit(digit); }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8 * theme.textScale

            OmaButton {
                Layout.fillWidth: true
                text: game.notesMode ? qsTr("Notes on") : qsTr("Notes")
                primary: game.notesMode
                onClicked: game.toggleNotesMode()
            }
            OmaButton {
                Layout.fillWidth: true
                text: qsTr("Erase")
                onClicked: game.erase()
            }
            OmaButton {
                Layout.fillWidth: true
                text: qsTr("Undo")
                enabled: game.canUndo
                onClicked: game.undo()
            }
            OmaButton {
                Layout.fillWidth: true
                text: qsTr("Restart")
                onClicked: game.restart()
            }
            OmaButton {
                Layout.fillWidth: true
                text: qsTr("New game")
                onClicked: root.leaveRequested()
            }
        }
    }
}
