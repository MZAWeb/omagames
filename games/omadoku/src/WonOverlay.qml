import QtQuick
import QtQuick.Layouts
import OmaGames

// Shown over the finished board.
Rectangle {
    id: root

    property string difficultyName: ""
    property int seconds: 0
    signal newGameRequested()
    signal backRequested()

    color: theme.alpha(theme.background, 0.85)

    // Swallow clicks and keys so the board underneath stays untouched.
    MouseArea { anchors.fill: parent }
    focus: true
    Keys.onPressed: function(event) { event.accepted = true; }

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
            OmaButton {
                Layout.fillWidth: true
                Layout.topMargin: 6 * theme.textScale
                text: qsTr("New game")
                primary: true
                onClicked: root.newGameRequested()
            }
            OmaButton {
                Layout.fillWidth: true
                text: qsTr("Back to start")
                onClicked: root.backRequested()
            }
        }
    }
}
