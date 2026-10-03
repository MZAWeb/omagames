import QtQuick
import QtQuick.Layouts
import OmaGames

OmaOverlayPanel {
    id: root

    readonly property bool perfect: game.gameOverReason === "filled"
    readonly property bool ranked: game.newHighScoreRank >= 0
    // A replay can stop with its snake still going: it was cut short there.
    readonly property bool cutShort: game.replaying && game.phase === "playing"

    // A replay is told about whoever played it, not to the one watching.
    function reasonText() {
        switch (game.gameOverReason) {
        case "self": return game.replaying ? qsTr("It ran into itself.") : qsTr("You ran into yourself.");
        case "filled": return game.replaying ? qsTr("It filled the board. Nothing left to eat.")
                                             : qsTr("You filled the board. Nothing left to eat.");
        }
        return game.replaying ? qsTr("It hit the wall.") : qsTr("You hit the wall.");
    }

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space || event.key === Qt.Key_R)
            game.restart();  // a replay's R is "watch again"
        else if (event.key === Qt.Key_Escape)
            game.backToStart();
        else
            return;
        event.accepted = true;
    }

    Text {
        Layout.alignment: Qt.AlignHCenter
        text: root.cutShort ? qsTr("End of the replay") : root.perfect ? qsTr("Perfect game!") : qsTr("Game over")
        color: root.cutShort ? theme.accent : root.perfect ? theme.green : theme.red
        font.pixelSize: 30 * theme.textScale
        font.bold: true
    }
    Text {
        Layout.fillWidth: true
        visible: !root.cutShort
        text: root.reasonText()
        color: theme.mix(theme.background, theme.foreground, 0.7)
        font.pixelSize: 13 * theme.textScale
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        text: game.score.toLocaleString(Qt.locale(), "f", 0)
        color: theme.foreground
        font.pixelSize: 34 * theme.textScale
        font.bold: true
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        text: qsTr("%1 · %2 · %3 long").arg(game.modeLabel).arg(game.difficultyLabel).arg(game.length)
        color: theme.mix(theme.background, theme.foreground, 0.7)
        font.pixelSize: 14 * theme.textScale
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        visible: game.replaying && game.replayAgent !== ""
        text: qsTr("Played by %1").arg(game.replayAgent)
        color: theme.mix(theme.background, theme.foreground, 0.7)
        font.pixelSize: 14 * theme.textScale
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        // A replay is not the player's to hold against their best.
        visible: !game.replaying
        text: root.ranked ? qsTr("New high score · #%1").arg(game.newHighScoreRank + 1)
                          : qsTr("Best %1").arg(game.best.toLocaleString(Qt.locale(), "f", 0))
        color: root.ranked ? theme.yellow : theme.mix(theme.background, theme.foreground, 0.7)
        font.pixelSize: 16 * theme.textScale
        font.bold: root.ranked
    }
    OmaHintButton {
        Layout.fillWidth: true
        Layout.topMargin: 6 * theme.textScale
        text: game.replaying ? qsTr("Watch again") : qsTr("Play again")
        primary: true
        hint: qsTr("Enter")
        onClicked: game.restart()
    }
    OmaHintButton {
        Layout.fillWidth: true
        text: qsTr("Back to start")
        hint: qsTr("Esc")
        onClicked: game.backToStart()
    }
}
