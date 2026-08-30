import QtQuick
import QtQuick.Layouts
import OmaGames

// Mode picker with each mode's best result, the high-score tables and a
// legend of the keys that matter once the well is up.
FocusScope {
    id: root

    property bool showingScores: false

    focus: true

    TimeFormat { id: clock }

    function bestText(mode) {
        var best = game.bests[mode.id];
        if (best <= 0)
            return "";
        return mode.goal > 0 ? qsTr("Best %1").arg(clock.text(best))
                             : qsTr("Best %1").arg(best.toLocaleString(Qt.locale(), "f", 0));
    }

    Keys.onPressed: function(event) {
        // The number keys pick the nth mode the game offers, so the list stays
        // the engine's to define.
        var mode = event.key - Qt.Key_1;
        if (mode >= 0 && mode < game.modes.length) {
            game.newGame(game.modes[mode].id);
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
            text: qsTr("Omatris")
            color: theme.foreground
            font.pixelSize: 40 * theme.textScale
            font.bold: true
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 10 * theme.textScale
            text: qsTr("Tetris for Omarchy")
            color: theme.mix(theme.background, theme.foreground, 0.6)
            font.pixelSize: 15 * theme.textScale
        }

        Repeater {
            model: game.modes

            ColumnLayout {
                id: entry
                required property var modelData
                required property int index

                Layout.fillWidth: true
                spacing: 3 * theme.textScale

                // Plain, every one of them: no key starts "the" mode, so
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
                        visible: entry.modelData.id === game.mode
                        text: qsTr("Last played")
                        color: theme.mix(theme.background, theme.foreground, 0.45)
                        font.pixelSize: 11 * theme.textScale
                        font.italic: true
                    }
                    Text {
                        text: root.bestText(entry.modelData)
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
