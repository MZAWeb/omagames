import QtQuick
import QtQuick.Layouts
import OmaGames

OmaOverlayPanel {
    id: root

    readonly property bool ranked: game.newHighScoreRank >= 0

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space || event.key === Qt.Key_R)
            game.restart();
        else if (event.key === Qt.Key_Escape)
            game.backToStart();
        else
            return;
        event.accepted = true;
    }

    Text {
        Layout.alignment: Qt.AlignHCenter
        text: qsTr("Reached the top")
        color: theme.red
        font.pixelSize: 30 * theme.textScale
        font.bold: true
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        text: game.score.toLocaleString(Qt.locale(), "f", 0)
        color: theme.foreground
        font.pixelSize: 42 * theme.textScale
        font.bold: true
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        text: qsTr("%1 shots · %2 pegs left").arg(game.shots).arg(game.pegCount)
        color: theme.mix(theme.background, theme.foreground, 0.68)
        font.pixelSize: 14 * theme.textScale
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        text: root.ranked ? qsTr("New high score · #%1").arg(game.newHighScoreRank + 1)
                          : qsTr("Best %1").arg(game.best.toLocaleString(Qt.locale(), "f", 0))
        color: root.ranked ? theme.yellow : theme.mix(theme.background, theme.foreground, 0.68)
        font.pixelSize: 16 * theme.textScale
        font.bold: root.ranked
    }
    OmaHintButton {
        Layout.fillWidth: true
        Layout.topMargin: 6 * theme.textScale
        text: qsTr("Play again")
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
