import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OmaGames

// Difficulty picker, the check mode setting and a way back into a saved game.
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
        } else if (event.key === Qt.Key_C) {
            game.checkAsYouGo = !game.checkAsYouGo;
        } else {
            return;
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

        Repeater {
            model: game.difficulties

            HintButton {
                required property var modelData
                required property int index

                Layout.fillWidth: true
                text: modelData.label
                primary: index === 0
                keyHint: (index + 1).toString()
                onClicked: game.newGame(modelData.id)
            }
        }
        HintButton {
            Layout.fillWidth: true
            Layout.topMargin: 10 * theme.textScale
            visible: game.hasSavedGame
            text: qsTr("Resume saved game")
            keyHint: qsTr("R")
            onClicked: game.resumeSavedGame()
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 12 * theme.textScale
            spacing: 8 * theme.textScale

            Switch {
                id: checkSwitch
                text: qsTr("Check as I go")
                checked: game.checkAsYouGo
                font.pixelSize: 14 * theme.textScale
                onToggled: game.checkAsYouGo = checked

                // The label is drawn by hand so it can take the theme's color;
                // `parent` inside a contentItem is only a plain Item, so the
                // control has to be reached through its id.
                contentItem: Text {
                    text: checkSwitch.text
                    color: theme.foreground
                    font: checkSwitch.font
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: checkSwitch.indicator.width + checkSwitch.spacing
                }
            }

            OmaKeyHint { key: qsTr("C") }
        }
    }
}
