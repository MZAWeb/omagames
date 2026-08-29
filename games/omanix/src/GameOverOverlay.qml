import QtQuick
import QtQuick.Layouts
import OmaGames

OverlayPanel {
    id: root

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_R)
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
        visible: game.newHighScoreRank >= 0
        text: game.newHighScoreRank === 0 ? qsTr("New best score!") : qsTr("High score #%1").arg(game.newHighScoreRank + 1)
        color: theme.yellow
        font.pixelSize: 16 * theme.textScale
        font.bold: true
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
