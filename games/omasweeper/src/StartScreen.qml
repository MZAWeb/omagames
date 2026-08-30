import QtQuick
import QtQuick.Layouts
import OmaGames

// Preset picker with each board's size, mine count and your best time, the
// best-time tables and a legend of the keys that matter once the field is up.
FocusScope {
    id: root

    property bool showingTimes: false

    focus: true

    function formatTime(seconds) {
        var rest = seconds % 60;
        return Math.floor(seconds / 60) + ":" + (rest < 10 ? "0" : "") + rest;
    }

    Keys.onPressed: function(event) {
        // The number keys pick the nth preset the game offers, so the list
        // stays the engine's to define.
        var preset = event.key - Qt.Key_1;
        if (preset >= 0 && preset < game.presets.length) {
            game.newGame(game.presets[preset].id);
        } else if (event.key === Qt.Key_H) {
            root.showingTimes = !root.showingTimes;
        } else if (event.key === Qt.Key_Escape && root.showingTimes) {
            root.showingTimes = false;
        } else {
            return;
        }
        event.accepted = true;
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48 * theme.textScale, 380 * theme.textScale)
        spacing: 10 * theme.textScale
        visible: !root.showingTimes

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Omasweeper")
            color: theme.foreground
            font.pixelSize: 40 * theme.textScale
            font.bold: true
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 10 * theme.textScale
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Minesweeper for Omarchy — no guessing, ever")
            color: theme.mix(theme.background, theme.foreground, 0.6)
            font.pixelSize: 15 * theme.textScale
            wrapMode: Text.WordWrap
        }

        Repeater {
            model: game.presets

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
                    primary: entry.modelData.id === game.preset
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
                        text: qsTr("%1×%2 · %3 mines").arg(entry.modelData.width)
                                  .arg(entry.modelData.height).arg(entry.modelData.mines)
                        color: theme.mix(theme.background, theme.foreground, 0.55)
                        font.pixelSize: 11 * theme.textScale
                    }
                    Text {
                        text: entry.best > 0 ? qsTr("Best %1").arg(root.formatTime(entry.best)) : ""
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
            text: qsTr("Best times")
            hint: qsTr("H")
            onClicked: root.showingTimes = true
        }

        KeyLegend {
            Layout.fillWidth: true
            Layout.topMargin: 14 * theme.textScale
        }
    }

    BestTimesPanel {
        anchors.centerIn: parent
        visible: root.showingTimes
        onCloseRequested: root.showingTimes = false
    }
}
