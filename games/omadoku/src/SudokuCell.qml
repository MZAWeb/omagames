import QtQuick

// One board cell: a digit, or its pencil marks, tinted by how it relates to
// the current selection. Knows nothing about the game object.
Item {
    id: cell

    property int value: 0
    property int notes: 0
    property bool given: false
    property bool wrong: false
    property bool selected: false
    property bool peer: false
    property bool sameDigit: false
    property real cellSize: width

    signal clicked()

    Rectangle {
        anchors.fill: parent
        color: cell.selected ? theme.alpha(theme.accent, 0.32)
             : cell.sameDigit ? theme.alpha(theme.accent, 0.14)
             : cell.peer ? theme.alpha(theme.foreground, 0.10)
             : "transparent"
        Behavior on color { ColorAnimation { duration: 80 } }
    }

    Text {
        anchors.centerIn: parent
        visible: cell.value > 0
        text: cell.value > 0 ? cell.value.toString() : ""
        color: cell.wrong ? theme.red : cell.given ? theme.foreground : theme.accent
        font.pixelSize: Math.max(1, Math.round(cell.cellSize * 0.6))
        font.bold: cell.given
    }

    Grid {
        anchors.fill: parent
        anchors.margins: Math.round(cell.cellSize * 0.06)
        columns: 3
        rows: 3
        visible: cell.value === 0 && cell.notes !== 0

        Repeater {
            model: 9
            Text {
                required property int index
                width: parent.width / 3
                height: parent.height / 3
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: (cell.notes & (1 << index)) ? (index + 1).toString() : ""
                color: theme.mix(theme.background, theme.foreground, 0.62)
                font.pixelSize: Math.max(1, Math.round(cell.cellSize * 0.26))
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: cell.clicked()
    }
}
