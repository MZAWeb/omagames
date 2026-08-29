import QtQuick
import QtQuick.Layouts
import OmaGames

// What a digit does — for the keypad and for the number row alike. The badge
// on the selected segment says the plain digits go there; the others show the
// whole chord that reaches them without switching mode, because a bare "Alt"
// does not say what to press it with.
RowLayout {
    id: root

    spacing: 6 * theme.textScale

    readonly property var modes: [
        {mode: "highlight", label: qsTr("Highlight"), chord: qsTr("Alt+1-9")},
        {mode: "note", label: qsTr("Note"), chord: qsTr("Shift+1-9")},
        {mode: "fill", label: qsTr("Fill"), chord: qsTr("Ctrl+1-9")},
    ]

    // A segment is a third of a column that shrinks with the window, and the
    // chord is wider than the name above it. Measure the widest of each row and
    // scale both fonts by the same factor, so a narrow window shrinks the type
    // instead of clipping it and the three segments stay the same size.
    readonly property real fit: {
        var room = group.width / 3 - 8 * theme.textScale;
        var cap = 8 * theme.textScale;  // OmaKeyHint pads its label
        return Math.max(0.6, Math.min(1,
            (room - cap) / chordMetrics.width, room / labelMetrics.width));
    }

    // iA Writer Mono S is monospaced, so the longest string is the widest one.
    function longest(field) {
        var widest = "";
        for (var i = 0; i < modes.length; ++i) {
            if (modes[i][field].length > widest.length)
                widest = modes[i][field];
        }
        return widest;
    }

    TextMetrics {
        id: chordMetrics
        font.pixelSize: 11 * theme.textScale
        font.bold: true
        text: root.longest("chord")
    }

    TextMetrics {
        id: labelMetrics
        font.pixelSize: 12 * theme.textScale
        font.bold: true
        text: root.longest("label")
    }

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
                            font.pixelSize: 12 * theme.textScale * root.fit
                            font.bold: selected
                        }
                        OmaKeyHint {
                            anchors.horizontalCenter: parent.horizontalCenter
                            key: selected ? qsTr("1-9") : modelData.chord
                            fontPixelSize: 11 * theme.textScale * root.fit
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
