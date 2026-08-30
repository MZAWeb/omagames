import QtQuick
import QtQuick.Layouts
import OmaGames

OmaPauseOverlay {
    id: root

    signal leaveRequested()

    subtitle: qsTr("Level %1 · %2% claimed").arg(game.level).arg(Math.floor(game.claimedPercent))

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Space || event.key === Qt.Key_P || event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
            game.resume();
        else if (event.key === Qt.Key_R)
            game.restartLevel();
        else if (event.key === Qt.Key_Escape)
            root.leaveRequested();
        else
            return;
        event.accepted = true;
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
        text: qsTr("Restart level")
        hint: qsTr("R")
        onClicked: game.restartLevel()
    }
    OmaHintButton {
        Layout.fillWidth: true
        text: qsTr("Leave game")
        hint: qsTr("Esc")
        onClicked: root.leaveRequested()
    }
}
