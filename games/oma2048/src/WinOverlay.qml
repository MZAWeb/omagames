import QtQuick
import QtQuick.Layouts
import OmaGames

// Up while `game.won` is: the run just made 2048 and Keep going has not been
// pressed. Closing it in any way other than starting over means keeping on.
OmaOverlayPanel {
    id: root

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_C || event.key === Qt.Key_Return
                || event.key === Qt.Key_Enter || event.key === Qt.Key_Escape)
            game.keepGoing();
        else if (event.key === Qt.Key_N)
            game.newGame();
        event.accepted = true;
    }

    Text {
        Layout.alignment: Qt.AlignHCenter
        text: qsTr("2048!")
        color: theme.accent
        font.pixelSize: 34 * theme.textScale
        font.bold: true
    }
    Text {
        Layout.fillWidth: true
        text: qsTr("You win. Keep going for the high score, or start fresh.")
        color: theme.mix(theme.background, theme.foreground, 0.7)
        font.pixelSize: 13 * theme.textScale
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        text: game.score.toLocaleString(Qt.locale(), "f", 0)
        color: theme.foreground
        font.pixelSize: 26 * theme.textScale
        font.bold: true
    }
    OmaHintButton {
        Layout.fillWidth: true
        Layout.topMargin: 6 * theme.textScale
        text: qsTr("Keep going")
        primary: true
        hint: qsTr("C")
        onClicked: game.keepGoing()
    }
    OmaHintButton {
        Layout.fillWidth: true
        text: qsTr("New game")
        hint: qsTr("N")
        onClicked: game.newGame()
    }
}
