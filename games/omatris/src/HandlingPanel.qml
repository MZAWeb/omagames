import QtQuick
import QtQuick.Layouts
import OmaGames

// How the keys feel: ↑↓ picks a setting, ←→ changes it, D puts every one back
// the way Omatris ships. The ranges, the values and their wording are the
// bridge's; this only lays them out and forwards keys.
OmaOverlayPanel {
    id: root

    property int current: 0

    signal closeRequested()

    maxWidth: 480 * theme.textScale
    dismissable: true
    onDismissed: root.closeRequested()

    function adjust(index, delta) {
        root.current = index;
        game.adjustHandling(game.handling[index].id, delta);
    }

    Keys.onPressed: function(event) {
        var count = game.handling.length;
        switch (event.key) {
        case Qt.Key_Up: root.current = (root.current + count - 1) % count; break;
        case Qt.Key_Down: root.current = (root.current + 1) % count; break;
        case Qt.Key_Left: root.adjust(root.current, -1); break;
        case Qt.Key_Right: root.adjust(root.current, 1); break;
        case Qt.Key_D: game.resetHandling(); break;
        case Qt.Key_S:
        case Qt.Key_Escape:
        case Qt.Key_Return:
        case Qt.Key_Enter:
            root.closeRequested();
            break;
        default: return;
        }
        event.accepted = true;
    }

    Text {
        Layout.alignment: Qt.AlignHCenter
        text: qsTr("Handling")
        color: theme.foreground
        font.pixelSize: 26 * theme.textScale
        font.bold: true
    }
    Text {
        Layout.fillWidth: true
        text: qsTr("How the keys feel. Changes apply at once and are kept.")
        color: theme.mix(theme.background, theme.foreground, 0.6)
        font.pixelSize: 13 * theme.textScale
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
    }

    Repeater {
        model: game.handling

        Rectangle {
            id: setting

            required property var modelData
            required property int index
            readonly property bool selected: index === root.current

            Layout.fillWidth: true
            implicitHeight: row.implicitHeight + 16 * theme.textScale
            radius: 8
            color: selected ? theme.alpha(theme.accent, 0.12) : "transparent"
            border.width: 1
            border.color: selected ? theme.alpha(theme.accent, 0.6) : "transparent"

            MouseArea {
                anchors.fill: parent
                onClicked: root.current = setting.index
            }

            RowLayout {
                id: row
                anchors.fill: parent
                anchors.margins: 8 * theme.textScale
                spacing: 8 * theme.textScale

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2 * theme.textScale

                    Text {
                        Layout.fillWidth: true
                        text: setting.modelData.label
                        color: theme.foreground
                        font.pixelSize: 14 * theme.textScale
                        font.bold: true
                    }
                    Text {
                        Layout.fillWidth: true
                        text: setting.modelData.description
                        color: theme.mix(theme.background, theme.foreground, 0.55)
                        font.pixelSize: 11 * theme.textScale
                        wrapMode: Text.WordWrap
                    }
                }
                OmaHintButton {
                    // Wide enough for the keycap, so the value column holds
                    // still as the selection moves.
                    Layout.preferredWidth: 52 * theme.textScale
                    text: qsTr("−")
                    hint: qsTr("←")
                    showHint: setting.selected
                    enabled: setting.modelData.canLower
                    onClicked: root.adjust(setting.index, -1)
                }
                // A value moved off its default takes the accent, so what was
                // changed is plain at a glance.
                Text {
                    Layout.preferredWidth: 64 * theme.textScale
                    text: setting.modelData.value
                    color: setting.modelData.isDefault ? theme.foreground : theme.accent
                    font.pixelSize: 14 * theme.textScale
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                }
                OmaHintButton {
                    Layout.preferredWidth: 52 * theme.textScale
                    text: qsTr("+")
                    hint: qsTr("→")
                    showHint: setting.selected
                    enabled: setting.modelData.canRaise
                    onClicked: root.adjust(setting.index, 1)
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 4 * theme.textScale
        spacing: 8 * theme.textScale

        OmaHintButton {
            Layout.fillWidth: true
            text: qsTr("Reset to defaults")
            hint: qsTr("D")
            enabled: !game.handlingIsDefault
            onClicked: game.resetHandling()
        }
        OmaHintButton {
            Layout.fillWidth: true
            text: qsTr("Done")
            primary: true
            hint: qsTr("Esc")
            onClicked: root.closeRequested()
        }
    }

    OmaKeyLegend {
        Layout.fillWidth: true
        compact: true
        model: [
            { key: qsTr("↑↓"), label: qsTr("choose") },
            { key: qsTr("←→"), label: qsTr("change") }
        ]
    }
}
