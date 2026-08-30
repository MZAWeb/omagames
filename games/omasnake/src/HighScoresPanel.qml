import QtQuick
import QtQuick.Layouts
import OmaGames

// The top ten of each speed, for one edges setting at a time: the same two
// groups as the start screen, so a table always says which rules it belongs to.
OmaPanel {
    id: root

    signal closeRequested()

    width: Math.min(parent.width - 48 * theme.textScale, 720 * theme.textScale)
    height: column.implicitHeight + 2 * padding

    function entriesFor(difficulty) {
        return game.highScores.filter(function(entry) {
            return entry.mode === game.mode && entry.difficulty === difficulty;
        });
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        spacing: 12 * theme.textScale

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("High scores")
            color: theme.foreground
            font.pixelSize: 22 * theme.textScale
            font.bold: true
        }

        EdgesSelector {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Math.min(root.width - 2 * root.padding, 320 * theme.textScale)
        }

        SectionHeading {
            Layout.fillWidth: true
            Layout.topMargin: 4 * theme.textScale
            text: qsTr("Speed")
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 16 * theme.textScale

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
                        Layout.alignment: Qt.AlignHCenter
                        text: table.modelData.label
                        color: theme.accent
                        font.pixelSize: 15 * theme.textScale
                        font.bold: true
                    }
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        visible: table.entries.length === 0
                        text: qsTr("No games yet")
                        color: theme.mix(theme.background, theme.foreground, 0.5)
                        font.pixelSize: 12 * theme.textScale
                    }
                    Repeater {
                        model: table.entries

                        RowLayout {
                            id: line
                            required property var modelData
                            required property int index
                            Layout.fillWidth: true
                            spacing: 6 * theme.textScale

                            Text {
                                text: (line.index + 1) + "."
                                color: theme.mix(theme.background, theme.foreground, 0.5)
                                font.pixelSize: 12 * theme.textScale
                                Layout.preferredWidth: 22 * theme.textScale
                            }
                            Text {
                                Layout.fillWidth: true
                                text: line.modelData.score.toLocaleString(Qt.locale(), "f", 0)
                                color: theme.foreground
                                font.pixelSize: 13 * theme.textScale
                                font.bold: line.index === 0
                            }
                            Text {
                                text: qsTr("%1 long").arg(line.modelData.length)
                                color: theme.mix(theme.background, theme.foreground, 0.6)
                                font.pixelSize: 12 * theme.textScale
                            }
                            Text {
                                text: line.modelData.date
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
