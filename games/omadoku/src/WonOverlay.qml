import QtQuick
import QtQuick.Layouts
import OmaGames

// Shown over the finished board.
Rectangle {
    id: root

    property string difficultyName: ""
    property int seconds: 0
    // Where this solve landed in the level's table (-1 = nowhere), and the
    // fastest time standing for it.
    property int rank: -1
    property int best: 0
    signal newGameRequested()
    signal backRequested()

    color: theme.alpha(theme.background, 0.85)

    function formatTime(seconds) {
        var rest = seconds % 60;
        return Math.floor(seconds / 60) + ":" + (rest < 10 ? "0" : "") + rest;
    }

    // Swallow clicks and keys so the board underneath stays untouched.
    MouseArea { anchors.fill: parent }
    focus: true
    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
            root.newGameRequested();
        else if (event.key === Qt.Key_Escape)
            root.backRequested();
        event.accepted = true;
    }

    OmaPanel {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48 * theme.textScale, 300 * theme.textScale)
        height: column.implicitHeight + 2 * padding

        ColumnLayout {
            id: column
            anchors.fill: parent
            spacing: 12 * theme.textScale

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Solved!")
                color: theme.green
                font.pixelSize: 30 * theme.textScale
                font.bold: true
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("%1 in %2").arg(root.difficultyName)
                          .arg(Math.floor(root.seconds / 60) + qsTr(" min ") + (root.seconds % 60) + qsTr(" s"))
                color: theme.mix(theme.background, theme.foreground, 0.7)
                font.pixelSize: 14 * theme.textScale
                horizontalAlignment: Text.AlignHCenter
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                visible: text !== ""
                text: root.rank === 0 ? qsTr("New best!")
                    : root.rank > 0 ? qsTr("#%1 for %2").arg(root.rank + 1).arg(root.difficultyName)
                    : root.best > 0 ? qsTr("Best %1").arg(root.formatTime(root.best))
                    : ""
                color: root.rank >= 0 ? theme.yellow : theme.mix(theme.background, theme.foreground, 0.7)
                font.pixelSize: 16 * theme.textScale
                font.bold: root.rank >= 0
                horizontalAlignment: Text.AlignHCenter
            }
            OmaHintButton {
                Layout.fillWidth: true
                Layout.topMargin: 6 * theme.textScale
                text: qsTr("New game")
                primary: true
                hint: qsTr("Enter")
                onClicked: root.newGameRequested()
            }
            OmaHintButton {
                Layout.fillWidth: true
                text: qsTr("Back to start")
                hint: qsTr("Esc")
                onClicked: root.backRequested()
            }
        }
    }
}
