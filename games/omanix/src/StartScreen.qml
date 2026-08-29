import QtQuick
import QtQuick.Layouts
import OmaGames

// Difficulty picker with each level's best score, the high-score table and a
// legend of the keys that matter once the field is up.
FocusScope {
    id: root

    property bool showingScores: false

    focus: true
    Keys.onPressed: function(event) {
        // The number keys pick the nth difficulty the game offers, so the list
        // stays the engine's to define.
        var level = event.key - Qt.Key_1;
        if (level >= 0 && level < game.difficulties.length) {
            game.newGame(game.difficulties[level].id);
        } else if (event.key === Qt.Key_H) {
            root.showingScores = !root.showingScores;
        } else if (event.key === Qt.Key_Escape && root.showingScores) {
            root.showingScores = false;
        } else {
            return;
        }
        event.accepted = true;
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48 * theme.textScale, 360 * theme.textScale)
        spacing: 10 * theme.textScale
        visible: !root.showingScores

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Omanix")
            color: theme.foreground
            font.pixelSize: 40 * theme.textScale
            font.bold: true
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 10 * theme.textScale
            text: qsTr("Xonix for Omarchy")
            color: theme.mix(theme.background, theme.foreground, 0.6)
            font.pixelSize: 15 * theme.textScale
        }

        Repeater {
            model: game.difficulties

            ColumnLayout {
                id: entry
                required property var modelData
                required property int index
                readonly property int best: game.bests[modelData.id]

                Layout.fillWidth: true
                spacing: 3 * theme.textScale

                OmaHintButton {
                    Layout.fillWidth: true
                    text: entry.modelData.label
                    primary: entry.modelData.id === game.difficulty
                    hint: (entry.index + 1).toString()
                    onClicked: game.newGame(entry.modelData.id)
                }
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 12 * theme.textScale
                    Layout.rightMargin: 12 * theme.textScale
                    Layout.bottomMargin: 4 * theme.textScale
                    Text {
                        Layout.fillWidth: true
                        text: entry.modelData.description
                        color: theme.mix(theme.background, theme.foreground, 0.55)
                        font.pixelSize: 11 * theme.textScale
                        wrapMode: Text.WordWrap
                    }
                    Text {
                        text: entry.best > 0 ? qsTr("Best %1").arg(entry.best.toLocaleString(Qt.locale(), "f", 0)) : ""
                        color: theme.mix(theme.background, theme.foreground, 0.55)
                        font.pixelSize: 11 * theme.textScale
                        font.bold: true
                    }
                }
            }
        }

        OmaHintButton {
            Layout.fillWidth: true
            Layout.topMargin: 10 * theme.textScale
            text: qsTr("High scores")
            hint: qsTr("H")
            onClicked: root.showingScores = true
        }

        KeyLegend {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 14 * theme.textScale
        }
    }

    HighScoresPanel {
        anchors.centerIn: parent
        visible: root.showingScores
        onCloseRequested: root.showingScores = false
    }
}
