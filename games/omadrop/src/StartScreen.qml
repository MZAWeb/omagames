import QtQuick
import QtQuick.Layouts
import OmaGames

FocusScope {
    id: root

    property bool showingScores: false
    focus: true

    Keys.onPressed: function(event) {
        if (!root.showingScores && (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space))
            game.newGame();
        else if (event.key === Qt.Key_H)
            root.showingScores = !root.showingScores;
        else if (event.key === Qt.Key_Escape && root.showingScores)
            root.showingScores = false;
        else
            return;
        event.accepted = true;
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48 * theme.textScale, 430 * theme.textScale)
        spacing: 12 * theme.textScale
        visible: !root.showingScores

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Omadrop")
            color: theme.foreground
            font.pixelSize: 42 * theme.textScale
            font.bold: true
        }
        Text {
            Layout.fillWidth: true
            text: qsTr("Aim, release, and keep the field clear.")
            color: theme.mix(theme.background, theme.foreground, 0.62)
            font.pixelSize: 15 * theme.textScale
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        OmaPanel {
            Layout.fillWidth: true
            Layout.topMargin: 14 * theme.textScale

            ColumnLayout {
                anchors.fill: parent
                spacing: 8 * theme.textScale
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Every hit counts a peg down and scores a point. After each ball, the field rises and one to three new pegs enter at the bottom. Let one reach the top and the run is over.")
                    color: theme.mix(theme.background, theme.foreground, 0.72)
                    font.pixelSize: 13 * theme.textScale
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                }
            }
        }

        OmaHintButton {
            Layout.fillWidth: true
            Layout.topMargin: 8 * theme.textScale
            text: qsTr("Play")
            primary: true
            hint: qsTr("Enter")
            onClicked: game.newGame()
        }
        OmaHintButton {
            Layout.fillWidth: true
            text: qsTr("High scores")
            hint: qsTr("H")
            onClicked: root.showingScores = true
        }

        KeyLegend {
            Layout.fillWidth: true
            Layout.topMargin: 14 * theme.textScale
            startScreen: true
        }
    }

    HighScoresPanel {
        anchors.centerIn: parent
        visible: root.showingScores
        onCloseRequested: root.showingScores = false
    }
}
