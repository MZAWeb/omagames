import QtQuick
import QtQuick.Layouts
import OmaGames

// The top ten of every difficulty side by side.
OmaPanel {
    id: root

    signal closeRequested()

    width: Math.min(parent.width - 48 * theme.textScale, 720 * theme.textScale)
    height: column.implicitHeight + 2 * padding

    function entriesFor(id) {
        return game.highScores.filter(function(entry) { return entry.difficulty === id; });
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
                            id: row
                            required property var modelData
                            required property int index
                            Layout.fillWidth: true
                            spacing: 6 * theme.textScale

                            Text {
                                text: (row.index + 1) + "."
                                color: theme.mix(theme.background, theme.foreground, 0.5)
                                font.pixelSize: 12 * theme.textScale
                                Layout.preferredWidth: 22 * theme.textScale
                            }
                            Text {
                                Layout.fillWidth: true
                                text: row.modelData.score.toLocaleString(Qt.locale(), "f", 0)
                                color: theme.foreground
                                font.pixelSize: 13 * theme.textScale
                                font.bold: row.index === 0
                            }
                            Text {
                                text: qsTr("L%1").arg(row.modelData.level)
                                color: theme.mix(theme.background, theme.foreground, 0.6)
                                font.pixelSize: 12 * theme.textScale
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
