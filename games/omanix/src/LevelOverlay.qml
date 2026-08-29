import QtQuick
import QtQuick.Layouts
import OmaGames

OverlayPanel {
    id: root

    function formatTime(seconds) {
        var rest = seconds % 60;
        return Math.floor(seconds / 60) + ":" + (rest < 10 ? "0" : "") + rest;
    }

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space)
            game.nextLevel();
        else
            return;
        event.accepted = true;
    }

    Text {
        Layout.alignment: Qt.AlignHCenter
        text: qsTr("Level %1 cleared!").arg(game.level)
        color: theme.green
        font.pixelSize: 28 * theme.textScale
        font.bold: true
    }
    GridLayout {
        Layout.alignment: Qt.AlignHCenter
        columns: 2
        columnSpacing: 18 * theme.textScale
        rowSpacing: 4 * theme.textScale

        Text { text: qsTr("Claimed"); color: theme.mix(theme.background, theme.foreground, 0.6); font.pixelSize: 14 * theme.textScale }
        Text { text: qsTr("%1%").arg(Math.floor(game.levelStats.percent)); color: theme.foreground; font.pixelSize: 14 * theme.textScale; font.bold: true }
        Text { text: qsTr("Lives bonus"); color: theme.mix(theme.background, theme.foreground, 0.6); font.pixelSize: 14 * theme.textScale }
        Text { text: "+" + game.levelStats.bonus.toLocaleString(Qt.locale(), "f", 0); color: theme.foreground; font.pixelSize: 14 * theme.textScale; font.bold: true }
        Text { text: qsTr("Time"); color: theme.mix(theme.background, theme.foreground, 0.6); font.pixelSize: 14 * theme.textScale }
        Text { text: root.formatTime(game.levelStats.seconds); color: theme.foreground; font.pixelSize: 14 * theme.textScale; font.bold: true }
        Text { text: qsTr("Score"); color: theme.mix(theme.background, theme.foreground, 0.6); font.pixelSize: 14 * theme.textScale }
        Text { text: game.score.toLocaleString(Qt.locale(), "f", 0); color: theme.foreground; font.pixelSize: 14 * theme.textScale; font.bold: true }
    }
    OmaHintButton {
        Layout.fillWidth: true
        Layout.topMargin: 6 * theme.textScale
        text: qsTr("Level %1").arg(game.level + 1)
        primary: true
        hint: qsTr("Enter")
        onClicked: game.nextLevel()
    }
}
