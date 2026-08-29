import QtQuick
import QtQuick.Layouts
import OmaGames

OverlayPanel {
    id: root

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space || event.key === Qt.Key_R)
            game.newGame(game.difficulty);
        else if (event.key === Qt.Key_Escape)
            game.backToStart();
        else
            return;
        event.accepted = true;
    }

    Text {
        Layout.alignment: Qt.AlignHCenter
        text: qsTr("Game over")
        color: theme.red
        font.pixelSize: 30 * theme.textScale
        font.bold: true
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
        text: qsTr("%1 · level %2").arg(game.difficultyLabel).arg(game.level)
        color: theme.mix(theme.background, theme.foreground, 0.7)
        font.pixelSize: 14 * theme.textScale
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        readonly property bool ranked: game.newHighScoreRank >= 0
        text: ranked ? qsTr("New high score · #%1").arg(game.newHighScoreRank + 1)
                     : qsTr("Best %1").arg(game.bests[game.difficulty].toLocaleString(Qt.locale(), "f", 0))
        color: ranked ? theme.yellow : theme.mix(theme.background, theme.foreground, 0.7)
        font.pixelSize: 16 * theme.textScale
        font.bold: ranked
    }
    OmaHintButton {
        Layout.fillWidth: true
        Layout.topMargin: 6 * theme.textScale
        text: qsTr("Play again")
        primary: true
        hint: qsTr("Enter")
        onClicked: game.newGame(game.difficulty)
    }
    OmaHintButton {
        Layout.fillWidth: true
        text: qsTr("Back to start")
        hint: qsTr("Esc")
        onClicked: game.backToStart()
    }
}
