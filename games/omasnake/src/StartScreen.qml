import QtQuick
import QtQuick.Layouts
import OmaGames

// Two settings kept apart: the edges switch, then the speed buttons that each
// start a game with the best score of that speed beside them. Below them the
// high-score table and a legend of the keys that matter once the field is up.
FocusScope {
    id: root

    property bool showingScores: false

    focus: true
    Keys.onPressed: function(event) {
        // The number keys pick the nth difficulty the game offers, so the list
        // stays the engine's to define.
        var speed = event.key - Qt.Key_1;
        if (!root.showingScores && speed >= 0 && speed < game.difficulties.length) {
            game.newGame(game.difficulties[speed].id);
        } else if (event.key === Qt.Key_M) {
            game.toggleMode();
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
        width: Math.min(parent.width - 48 * theme.textScale, 380 * theme.textScale)
        spacing: 10 * theme.textScale
        visible: !root.showingScores

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Omasnake")
            color: theme.foreground
            font.pixelSize: 40 * theme.textScale
            font.bold: true
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Snake for Omarchy")
            color: theme.mix(theme.background, theme.foreground, 0.6)
            font.pixelSize: 15 * theme.textScale
        }

        EdgesSelector {
            Layout.fillWidth: true
            Layout.topMargin: 12 * theme.textScale
        }

        SectionHeading {
            Layout.fillWidth: true
            Layout.topMargin: 12 * theme.textScale
            Layout.bottomMargin: 2 * theme.textScale
            text: qsTr("Speed")
        }

        Repeater {
            model: game.difficulties

            ColumnLayout {
                id: entry
                required property var modelData
                required property int index
                readonly property int best: game.bests[game.mode + "-" + modelData.id]

                Layout.fillWidth: true
                spacing: 3 * theme.textScale

                // Plain, every one of them: no key starts "the" speed, so
                // nothing here may look like what Enter would press.
                OmaHintButton {
                    Layout.fillWidth: true
                    text: entry.modelData.label
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
                        visible: entry.modelData.id === game.difficulty
                        text: qsTr("Last played")
                        color: theme.mix(theme.background, theme.foreground, 0.45)
                        font.pixelSize: 11 * theme.textScale
                        font.italic: true
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
            Layout.fillWidth: true
            Layout.topMargin: 14 * theme.textScale
        }
    }

    HighScoresPanel {
        anchors.centerIn: parent
        visible: root.showingScores
        onCloseRequested: root.showingScores = false
    }
}
