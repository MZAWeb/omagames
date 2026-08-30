import QtQuick
import QtQuick.Layouts
import OmaGames

// The board is solved: the clock, where the time landed, and the way out.
// The cover is light so the finished board stays readable behind it.
OverlayPanel {
    id: root

    readonly property bool ranked: game.newBestRank >= 0
    readonly property int best: game.bests[game.preset]

    dim: 0.6

    function formatTime(seconds) {
        var rest = seconds % 60;
        return Math.floor(seconds / 60) + ":" + (rest < 10 ? "0" : "") + rest;
    }

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space
            || event.key === Qt.Key_R)
            game.restart();
        else if (event.key === Qt.Key_Escape)
            game.backToStart();
        else
            return;
        event.accepted = true;
    }

    Text {
        Layout.alignment: Qt.AlignHCenter
        text: qsTr("Swept!")
        color: theme.green
        font.pixelSize: 30 * theme.textScale
        font.bold: true
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        text: root.formatTime(game.elapsedSeconds)
        color: theme.foreground
        font.pixelSize: 34 * theme.textScale
        font.bold: true
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        text: qsTr("%1 · %2 mines").arg(game.presetLabel).arg(game.mines)
        color: theme.mix(theme.background, theme.foreground, 0.7)
        font.pixelSize: 14 * theme.textScale
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        text: root.ranked ? qsTr("New best time · #%1").arg(game.newBestRank + 1)
                          : qsTr("Best %1").arg(root.formatTime(root.best))
        color: root.ranked ? theme.yellow : theme.mix(theme.background, theme.foreground, 0.7)
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
