import QtQuick
import QtQuick.Layouts
import OmaGames

// What a click on the keypad does — and nothing else. The caption says so
// outright, and the segments wear no key badges precisely because the keyboard
// does not go through here: its mapping is fixed and spelled out by KeyboardMap.
ColumnLayout {
    id: root

    spacing: 3 * theme.textScale

    readonly property var modes: [
        {mode: "highlight", label: qsTr("Highlight")},
        {mode: "note", label: qsTr("Note")},
        {mode: "fill", label: qsTr("Fill")},
    ]

    // A segment is a third of a column that shrinks with the window, so measure
    // the widest name and scale every segment's font by the same factor: a
    // narrow window shrinks the type instead of clipping it.
    readonly property real fit: {
        var room = group.width / 3 - 8 * theme.textScale;
        return Math.max(0.6, Math.min(1, room / labelMetrics.width));
    }

    // iA Writer Mono S is monospaced, so the longest label is the widest one.
    readonly property string widestLabel: {
        var widest = "";
        for (var i = 0; i < modes.length; ++i) {
            if (modes[i].label.length > widest.length)
                widest = modes[i].label;
        }
        return widest;
    }

    TextMetrics {
        id: labelMetrics
        font.pixelSize: 12 * theme.textScale
        font.bold: true
        text: root.widestLabel
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 6 * theme.textScale

        Text {
            text: qsTr("Mouse clicks")
            color: theme.mix(theme.background, theme.foreground, 0.5)
            font.pixelSize: 11 * theme.textScale
        }
        Item { Layout.fillWidth: true }
        OmaKeyHint { key: qsTr("N") }
    }

    Rectangle {
        id: group

        Layout.fillWidth: true
        implicitHeight: Math.round(38 * theme.textScale)
        radius: Math.round(8 * theme.textScale)
        color: "transparent"
        border.width: 1
        border.color: theme.alpha(theme.foreground, 0.25)
        clip: true  // squares off the inner segments against the rounded frame

        Row {
            anchors.fill: parent

            Repeater {
                model: root.modes

                Item {
                    required property var modelData
                    required property int index
                    readonly property bool selected: game.clickMode === modelData.mode

                    width: group.width / 3
                    height: group.height

                    Rectangle {
                        anchors.fill: parent
                        color: parent.selected ? theme.accent
                             : mouse.containsMouse ? theme.alpha(theme.foreground, 0.10)
                             : "transparent"
                        Behavior on color { ColorAnimation { duration: 90 } }
                    }

                    Rectangle {
                        visible: parent.index > 0
                        width: 1
                        height: parent.height
                        color: theme.alpha(theme.foreground, 0.25)
                    }

                    Text {
                        anchors.centerIn: parent
                        text: modelData.label
                        color: selected ? theme.background : theme.foreground
                        font.pixelSize: 12 * theme.textScale * root.fit
                        font.bold: selected
                    }

                    MouseArea {
                        id: mouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: game.clickMode = modelData.mode
                    }
                }
            }
        }
    }
}
