import QtQuick
import QtQuick.Layouts

// What the field's edges do, as one two-segment switch: the lit segment is the
// setting and the line under it says what that choice costs you. A switch, not
// a button, because it is not something you start a game with.
ColumnLayout {
    id: root

    spacing: 5 * theme.textScale

    readonly property var chosen: game.modes.filter(function(entry) { return entry.id === game.mode; })

    SectionHeading {
        Layout.fillWidth: true
        text: qsTr("Edges")
        hint: qsTr("M")
    }

    Rectangle {
        id: group

        Layout.fillWidth: true
        implicitHeight: Math.round(38 * theme.textScale)
        radius: Math.round(8 * theme.textScale)
        color: "transparent"
        border.width: 1
        border.color: theme.alpha(theme.foreground, 0.25)

        // The end segments carry the frame's corner radius, so the selected one
        // fills the rounded shape instead of squaring it off. Clipping cannot
        // do this — Qt clips to the bounding rectangle, corners and all.
        Row {
            id: segments

            anchors.fill: parent
            anchors.margins: group.border.width

            Repeater {
                model: game.modes

                Item {
                    id: segment
                    required property var modelData
                    required property int index
                    readonly property bool selected: game.mode === modelData.id
                    readonly property real firstRadius: index === 0 ? group.radius - group.border.width : 0
                    readonly property real lastRadius:
                        index === game.modes.length - 1 ? group.radius - group.border.width : 0

                    width: segments.width / game.modes.length
                    height: segments.height

                    Rectangle {
                        anchors.fill: parent
                        topLeftRadius: segment.firstRadius
                        bottomLeftRadius: segment.firstRadius
                        topRightRadius: segment.lastRadius
                        bottomRightRadius: segment.lastRadius
                        color: segment.selected ? theme.accent
                             : mouse.containsMouse ? theme.alpha(theme.foreground, 0.10)
                             : "transparent"
                        Behavior on color { ColorAnimation { duration: 90 } }
                    }

                    Rectangle {
                        visible: segment.index > 0
                        width: 1
                        height: segment.height
                        color: theme.alpha(theme.foreground, 0.25)
                    }

                    Text {
                        anchors.centerIn: parent
                        text: segment.modelData.label
                        color: segment.selected ? theme.background : theme.foreground
                        font.pixelSize: 13 * theme.textScale
                        font.bold: segment.selected
                    }

                    MouseArea {
                        id: mouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: game.setMode(segment.modelData.id)
                    }
                }
            }
        }
    }

    Text {
        Layout.fillWidth: true
        Layout.leftMargin: 2 * theme.textScale
        Layout.rightMargin: 2 * theme.textScale
        text: root.chosen.length > 0 ? root.chosen[0].description : ""
        color: theme.mix(theme.background, theme.foreground, 0.55)
        font.pixelSize: 11 * theme.textScale
        wrapMode: Text.WordWrap
    }
}
