import QtQuick
import QtQuick.Layouts
import OmaGames

// The five fastest solves of every level, side by side.
OmaPanel {
    id: root

    signal closeRequested()

    width: Math.min(parent.width - 48 * theme.textScale, 660 * theme.textScale)
    height: column.implicitHeight + 2 * padding

    function entriesFor(id) {
        return game.bestTimes.filter(function(entry) { return entry.difficulty === id; });
    }

    function formatTime(seconds) {
        var rest = seconds % 60;
        return Math.floor(seconds / 60) + ":" + (rest < 10 ? "0" : "") + rest;
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        spacing: 12 * theme.textScale

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Best times")
            color: theme.foreground
            font.pixelSize: 22 * theme.textScale
            font.bold: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 14 * theme.textScale

            Repeater {
                model: game.difficulties

                ColumnLayout {
                    id: table
                    required property var modelData
                    readonly property var entries: root.entriesFor(modelData.id)

                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    spacing: 4 * theme.textScale

                    Text {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: table.modelData.label
                        color: theme.accent
                        font.pixelSize: 15 * theme.textScale
                        font.bold: true
                        elide: Text.ElideRight
                    }
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        visible: table.entries.length === 0
                        text: qsTr("Never solved")
                        color: theme.mix(theme.background, theme.foreground, 0.5)
                        font.pixelSize: 12 * theme.textScale
                    }
                    Repeater {
                        model: table.entries

                        RowLayout {
                            id: row
                            required property var modelData
                            required property int index

                            Layout.fillWidth: true
                            spacing: 6 * theme.textScale

                            Text {
                                Layout.preferredWidth: 20 * theme.textScale
                                text: (row.index + 1) + "."
                                color: theme.mix(theme.background, theme.foreground, 0.5)
                                font.pixelSize: 12 * theme.textScale
                            }
                            Text {
                                Layout.fillWidth: true
                                text: root.formatTime(row.modelData.seconds)
                                color: theme.foreground
                                font.pixelSize: 13 * theme.textScale
                                font.bold: row.index === 0
                            }
                            Text {
                                text: row.modelData.date
                                color: theme.mix(theme.background, theme.foreground, 0.5)
                                font.pixelSize: 11 * theme.textScale
                            }
                        }
                    }
                }
            }
        }

        OmaHintButton {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 6 * theme.textScale
            text: qsTr("Back")
            hint: qsTr("Esc")
            onClicked: root.closeRequested()
        }
    }
}
