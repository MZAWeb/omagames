import QtQuick
import QtQuick.Layouts
import OmaGames

// A mine went off. Every mine is turned over on the board behind, wrong
// flags crossed out, so the cover stays light enough to read them.
OverlayPanel {
    id: root

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
        text: qsTr("Boom")
        color: theme.red
        font.pixelSize: 30 * theme.textScale
        font.bold: true
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        text: qsTr("%1 · %2 flags · %3").arg(game.presetLabel).arg(game.flags)
                  .arg(root.formatTime(game.elapsedSeconds))
        color: theme.mix(theme.background, theme.foreground, 0.7)
        font.pixelSize: 14 * theme.textScale
        horizontalAlignment: Text.AlignHCenter
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        text: game.noGuess ? qsTr("This board never needed a guess.")
                           : qsTr("This board came with no promise.")
        color: theme.mix(theme.background, theme.foreground, 0.55)
        font.pixelSize: 12 * theme.textScale
    }
    OmaHintButton {
        Layout.fillWidth: true
        Layout.topMargin: 6 * theme.textScale
        text: qsTr("New board")
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
