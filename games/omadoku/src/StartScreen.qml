import QtQuick
import QtQuick.Layouts
import OmaGames

// Difficulty picker and a way back into a saved game.
FocusScope {
    id: root

    focus: true
    Keys.onPressed: function(event) {
        // The number keys pick the nth difficulty the game offers, so the list
        // stays the engine's to define.
        var level = event.key - Qt.Key_1;
        if (level >= 0 && level < game.difficulties.length) {
            game.newGame(game.difficulties[level].id);
        } else if (event.key === Qt.Key_R) {
            if (game.hasSavedGame)
                game.resumeSavedGame();
        } else {
            return;
        }
        event.accepted = true;
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48 * theme.textScale, 340 * theme.textScale)
        spacing: 10 * theme.textScale

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

        Repeater {
            model: game.difficulties

            // A level is its button and, under it, the techniques it asks for
            // beyond the level above: the promise the generator keeps.
            ColumnLayout {
                required property var modelData
                required property int index

                Layout.fillWidth: true
                spacing: 3 * theme.textScale

                OmaHintButton {
                    Layout.fillWidth: true
                    text: parent.modelData.label
                    primary: parent.index === 0
                    hint: (parent.index + 1).toString()
                    onClicked: game.newGame(parent.modelData.id)
                }
                Text {
                    Layout.fillWidth: true
                    Layout.leftMargin: 12 * theme.textScale
                    Layout.rightMargin: 12 * theme.textScale
                    Layout.bottomMargin: 4 * theme.textScale
                    text: parent.modelData.techniques.join(" · ")
                    color: theme.mix(theme.background, theme.foreground, 0.55)
                    font.pixelSize: 11 * theme.textScale
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
        OmaHintButton {
            Layout.fillWidth: true
            Layout.topMargin: 10 * theme.textScale
            visible: game.hasSavedGame
            text: qsTr("Resume saved game")
            hint: qsTr("R")
            onClicked: game.resumeSavedGame()
        }
    }
}
