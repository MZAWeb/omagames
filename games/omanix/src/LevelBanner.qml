import QtQuick
import OmaGames

// "Level 3 · 5 balls · 2 chasers", over the field while the level intro
// holds everything still.
OmaPanel {
    id: banner

    property bool shown: false

    function plural(count, one, many) {
        return count + " " + (count === 1 ? one : many);
    }

    width: label.implicitWidth + 2 * padding
    height: label.implicitHeight + 2 * padding
    padding: 14 * theme.textScale
    opacity: shown ? 1 : 0
    visible: opacity > 0
    scale: shown ? 1 : 0.9
    Behavior on opacity { NumberAnimation { duration: 160 } }
    Behavior on scale { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }

    Text {
        id: label
        anchors.centerIn: parent
        text: qsTr("Level %1 · %2 · %3").arg(game.level)
                  .arg(banner.plural(game.ballCount, qsTr("ball"), qsTr("balls")))
                  .arg(banner.plural(game.chaserCount, qsTr("chaser"), qsTr("chasers")))
        color: theme.foreground
        font.pixelSize: 20 * theme.textScale
        font.bold: true
    }
}
