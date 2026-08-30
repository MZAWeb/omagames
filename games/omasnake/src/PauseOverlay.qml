import QtQuick
import QtQuick.Layouts
import OmaGames

OverlayPanel {
    id: root

    signal leaveRequested()

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Space || event.key === Qt.Key_P || event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
            game.resume();
        else if (event.key === Qt.Key_R)
            game.restart();
        else if (event.key === Qt.Key_Escape)
            root.leaveRequested();
        else
            return;
        event.accepted = true;
    }

    Text {
        Layout.alignment: Qt.AlignHCenter
        text: qsTr("Paused")
        color: theme.foreground
        font.pixelSize: 30 * theme.textScale
        font.bold: true
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        text: qsTr("%1 points · %2 long").arg(game.score.toLocaleString(Qt.locale(), "f", 0)).arg(game.length)
        color: theme.mix(theme.background, theme.foreground, 0.7)
        font.pixelSize: 14 * theme.textScale
    }
    OmaHintButton {
        Layout.fillWidth: true
        Layout.topMargin: 6 * theme.textScale
        text: qsTr("Resume")
        primary: true
        hint: qsTr("Space")
        onClicked: game.resume()
    }
    OmaHintButton {
        Layout.fillWidth: true
        text: qsTr("Restart")
        hint: qsTr("R")
        onClicked: game.restart()
    }
    OmaHintButton {
        Layout.fillWidth: true
        text: qsTr("Leave game")
        hint: qsTr("Esc")
        onClicked: root.leaveRequested()
    }
}
