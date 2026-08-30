import QtQuick
import OmaGames

// "Ready · Classic · Normal", over the field while the opening beat holds
// the snake still.
OmaPanel {
    id: banner

    property bool shown: false

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
        text: qsTr("Ready · %1 · %2").arg(game.modeLabel).arg(game.difficultyLabel)
        color: theme.foreground
        font.pixelSize: 20 * theme.textScale
        font.bold: true
    }
}
