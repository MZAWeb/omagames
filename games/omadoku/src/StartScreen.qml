import QtQuick
import QtQuick.Layouts
import OmaGames

// Difficulty picker, each level's best time, and a way back into a saved game.
FocusScope {
    id: root

    property bool showingTimes: false

    focus: true

    function formatTime(seconds) {
        var rest = seconds % 60;
        return Math.floor(seconds / 60) + ":" + (rest < 10 ? "0" : "") + rest;
    }

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_B) {
            root.showingTimes = !root.showingTimes;
        } else if (event.key === Qt.Key_Escape && root.showingTimes) {
            root.showingTimes = false;
        } else if (root.showingTimes) {
            return;  // the panel is modal enough that nothing else applies
        // The number keys pick the nth difficulty the game offers, so the list
        // stays the engine's to define.
        } else if (event.key - Qt.Key_1 >= 0 && event.key - Qt.Key_1 < game.difficulties.length) {
            game.newGame(game.difficulties[event.key - Qt.Key_1].id);
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
        visible: !root.showingTimes

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
            // beyond the level above — the promise the generator keeps — and
            // the fastest you have solved it.
            ColumnLayout {
                id: level
                required property var modelData
                required property int index
                readonly property int best: game.bests[modelData.id]

                Layout.fillWidth: true
                spacing: 3 * theme.textScale

                // Plain, like every other level: an accent fill would promise
                // that Enter picks this one, and nothing here answers Enter.
                OmaHintButton {
                    Layout.fillWidth: true
                    text: level.modelData.label
                    hint: (level.index + 1).toString()
                    onClicked: game.newGame(level.modelData.id)
                }
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 12 * theme.textScale
                    Layout.rightMargin: 12 * theme.textScale
                    Layout.bottomMargin: 4 * theme.textScale
                    spacing: 8 * theme.textScale

                    Text {
                        Layout.fillWidth: true
                        text: level.modelData.techniques.join(" · ")
                        color: theme.mix(theme.background, theme.foreground, 0.55)
                        font.pixelSize: 11 * theme.textScale
                        wrapMode: Text.WordWrap
                    }
                    Text {
                        Layout.alignment: Qt.AlignTop
                        visible: level.best > 0
                        text: qsTr("best %1").arg(root.formatTime(level.best))
                        color: theme.mix(theme.background, theme.foreground, 0.55)
                        font.pixelSize: 11 * theme.textScale
                        font.bold: true
                    }
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
        OmaHintButton {
            Layout.fillWidth: true
            text: qsTr("Best times")
            hint: qsTr("B")
            onClicked: root.showingTimes = true
        }
    }

    BestTimesPanel {
        anchors.centerIn: parent
        visible: root.showingTimes
        onCloseRequested: root.showingTimes = false
    }
}
