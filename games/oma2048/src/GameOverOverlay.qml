import QtQuick
import QtQuick.Layouts
import OmaGames

// The run is out of moves; its score was just recorded. Names the rank when
// the run made the kept table.
OmaOverlayPanel {
    id: root

    signal scoresRequested()

    readonly property bool ranked: game.lastRank >= 0

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter
                || event.key === Qt.Key_Space || event.key === Qt.Key_N)
            game.newGame();
        else if (event.key === Qt.Key_S)
            root.scoresRequested();
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
        Layout.fillWidth: true
        text: qsTr("No moves left.")
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
        text: root.ranked ? qsTr("New high score · #%1").arg(game.lastRank + 1)
                          : qsTr("Best %1").arg(game.bestScore.toLocaleString(Qt.locale(), "f", 0))
        color: root.ranked ? theme.yellow : theme.mix(theme.background, theme.foreground, 0.7)
        font.pixelSize: 16 * theme.textScale
        font.bold: root.ranked
    }
    OmaHintButton {
        Layout.fillWidth: true
        Layout.topMargin: 6 * theme.textScale
        text: qsTr("New game")
        primary: true
        hint: qsTr("Enter")
        onClicked: game.newGame()
    }
    OmaHintButton {
        Layout.fillWidth: true
        text: qsTr("High scores")
        hint: qsTr("S")
        onClicked: root.scoresRequested()
    }
}
