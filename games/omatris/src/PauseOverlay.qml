import QtQuick
import QtQuick.Layouts
import OmaGames

OmaPauseOverlay {
    id: root

    signal leaveRequested()

    TimeFormat { id: clock }

    subtitle: game.rankByTime
        ? qsTr("%1 · %2 lines left · %3").arg(game.modeLabel).arg(game.linesLeft).arg(clock.text(game.elapsedMs))
        : qsTr("%1 · level %2 · %3 lines").arg(game.modeLabel).arg(game.level).arg(game.lines)

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_P || event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
            game.resume();
        else if (event.key === Qt.Key_R)
            game.restart();
        else if (event.key === Qt.Key_G)
            game.toggleGhost();
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
        hint: qsTr("P")
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
        text: game.ghostEnabled ? qsTr("Ghost on") : qsTr("Ghost off")
        hint: qsTr("G")
        onClicked: game.toggleGhost()
    }
    OmaHintButton {
        Layout.fillWidth: true
        text: qsTr("Leave game")
        hint: qsTr("Esc")
        onClicked: root.leaveRequested()
    }
}
