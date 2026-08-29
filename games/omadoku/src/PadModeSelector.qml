import QtQuick
import QtQuick.Layouts
import OmaGames

// What a digit does — for the keypad and for the number row alike. The badge
// on the selected segment says the plain digits go there; the others show the
// modifier that reaches them without switching mode.
RowLayout {
    id: root

    spacing: 6 * theme.textScale

    readonly property var modes: [
        {mode: "highlight", label: qsTr("Highlight"), modifier: qsTr("Alt")},
        {mode: "note", label: qsTr("Note"), modifier: qsTr("Shift")},
        {mode: "fill", label: qsTr("Fill"), modifier: qsTr("Ctrl")},
    ]

    Rectangle {
        id: group

        Layout.fillWidth: true
        implicitHeight: Math.round(46 * theme.textScale)
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
                    readonly property bool selected: game.padMode === modelData.mode

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

                    Column {
                        anchors.centerIn: parent
                        spacing: 2 * theme.textScale

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: modelData.label
                            color: selected ? theme.background : theme.foreground
                            font.pixelSize: 12 * theme.textScale
                            font.bold: selected
                        }
                        OmaKeyHint {
                            anchors.horizontalCenter: parent.horizontalCenter
                            key: selected ? qsTr("1-9") : modelData.modifier
                            onPrimary: selected
                        }
                    }

                    MouseArea {
                        id: mouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: game.padMode = modelData.mode
                    }
                }
            }
        }
    }

    OmaKeyHint { key: qsTr("N") }
}
