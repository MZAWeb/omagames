import QtQuick
import QtQuick.Layouts
import OmaGames

// The kept table of every category side by side: high scores, best times.
// `entries` is one flat list — what a bridge builds from ScoreTable — and
// each row says which category it belongs to under `categoryField`.
// `valueMode` picks how the ranked number reads; a game whose categories
// disagree passes `valueText` instead. Anything a game wants between the
// heading and the tables it adds as its own children.
OmaPanel {
    id: root

    default property alias header: headerBox.data

    property string title: ""
    property real maxWidth: 720 * theme.textScale
    property real tableSpacing: 16 * theme.textScale
    property real rankWidth: 22 * theme.textScale
    // [{id, label}, ...], in the order the tables are shown.
    property var categories: []
    property var entries: []
    property string categoryField: "difficulty"
    property string emptyText: qsTr("No games yet")
    // "score" for a plain number, "time" for a m:ss clock in `seconds`.
    property string valueMode: "score"
    // Optional function(entry, category) overrides for the ranked number and
    // for the column beside it; no `detailText` means no such column.
    property var valueText: null
    property var detailText: null
    // Omadoku's level names are long enough to need the room.
    property bool elideLabels: false
    signal closeRequested()

    width: Math.min(parent.width - 48 * theme.textScale, maxWidth)
    height: column.implicitHeight + 2 * padding

    function rowsFor(id) {
        var field = root.categoryField;
        return root.entries.filter(function(entry) { return entry[field] === id; });
    }

    function numberText(value) {
        return Number(value).toLocaleString(Qt.locale(), "f", 0);
    }

    function clockText(seconds) {
        var rest = seconds % 60;
        return Math.floor(seconds / 60) + ":" + (rest < 10 ? "0" : "") + rest;
    }

    function rankedText(entry, category) {
        if (root.valueText)
            return root.valueText(entry, category);
        return root.valueMode === "time" ? root.clockText(entry.seconds) : root.numberText(entry.score);
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        spacing: 12 * theme.textScale

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: root.title
            color: theme.foreground
            font.pixelSize: 22 * theme.textScale
            font.bold: true
        }

        ColumnLayout {
            id: headerBox
            Layout.fillWidth: true
            visible: children.length > 0
            spacing: 12 * theme.textScale
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: root.tableSpacing

            Repeater {
                model: root.categories

                ColumnLayout {
                    id: table
                    required property var modelData
                    readonly property var rows: root.rowsFor(modelData.id)

                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    spacing: 4 * theme.textScale

                    Text {
                        Layout.fillWidth: root.elideLabels
                        Layout.alignment: Qt.AlignHCenter
                        horizontalAlignment: Text.AlignHCenter
                        text: table.modelData.label
                        color: theme.accent
                        font.pixelSize: 15 * theme.textScale
                        font.bold: true
                        elide: root.elideLabels ? Text.ElideRight : Text.ElideNone
                    }
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        visible: table.rows.length === 0
                        text: root.emptyText
                        color: theme.mix(theme.background, theme.foreground, 0.5)
                        font.pixelSize: 12 * theme.textScale
                    }
                    Repeater {
                        model: table.rows

                        RowLayout {
                            id: row
                            required property var modelData
                            required property int index

                            Layout.fillWidth: true
                            spacing: 6 * theme.textScale

                            Text {
                                Layout.preferredWidth: root.rankWidth
                                text: (row.index + 1) + "."
                                color: theme.mix(theme.background, theme.foreground, 0.5)
                                font.pixelSize: 12 * theme.textScale
                            }
                            Text {
                                Layout.fillWidth: true
                                text: root.rankedText(row.modelData, table.modelData)
                                color: theme.foreground
                                font.pixelSize: 13 * theme.textScale
                                font.bold: row.index === 0
                            }
                            Text {
                                visible: root.detailText !== null
                                text: root.detailText ? root.detailText(row.modelData, table.modelData) : ""
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
