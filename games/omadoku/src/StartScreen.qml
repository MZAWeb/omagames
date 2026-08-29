import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaGames
import Omadoku

// Difficulty picker, the check mode setting and a way back into a saved game.
FocusScope {
    id: root

    focus: true
    Keys.onPressed: function(event) {
        switch (event.key) {
        case Qt.Key_1: game.newGame(SudokuGame.Easy); break;
        case Qt.Key_2: game.newGame(SudokuGame.Medium); break;
        case Qt.Key_3: game.newGame(SudokuGame.Hard); break;
        case Qt.Key_R: if (game.hasSavedGame) game.resumeSavedGame(); break;
        default: return;
        }
        event.accepted = true;
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48 * theme.textScale, 320 * theme.textScale)
        spacing: 14 * theme.textScale

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Omadoku")
            color: theme.foreground
            font.pixelSize: 40 * theme.textScale
            font.bold: true
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 10 * theme.textScale
            text: qsTr("Sudoku for Omarchy")
            color: theme.mix(theme.background, theme.foreground, 0.6)
            font.pixelSize: 15 * theme.textScale
        }

        OmaButton {
            Layout.fillWidth: true
            text: qsTr("Easy")
            primary: true
            onClicked: game.newGame(SudokuGame.Easy)
        }
        OmaButton {
            Layout.fillWidth: true
            text: qsTr("Medium")
            onClicked: game.newGame(SudokuGame.Medium)
        }
        OmaButton {
            Layout.fillWidth: true
            text: qsTr("Hard")
            onClicked: game.newGame(SudokuGame.Hard)
        }
        OmaButton {
            Layout.fillWidth: true
            Layout.topMargin: 10 * theme.textScale
            visible: game.hasSavedGame
            text: qsTr("Resume saved game")
            onClicked: game.resumeSavedGame()
        }

        Switch {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 12 * theme.textScale
            text: qsTr("Check as I go")
            checked: game.checkAsYouGo
            font.pixelSize: 14 * theme.textScale
            onToggled: game.checkAsYouGo = checked

            contentItem: Text {
                text: parent.text
                color: theme.foreground
                font: parent.font
                verticalAlignment: Text.AlignVCenter
                leftPadding: parent.indicator.width + parent.spacing
            }
        }
    }
}
