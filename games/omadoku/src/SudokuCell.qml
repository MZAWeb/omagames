import QtQuick

// One board cell: a digit, or its pencil marks, tinted by how it relates to
// the current selection. Knows nothing about the game object.
//
// Feedback stays local to the cell that changed — a placement settles, a note
// the placement retracted fades out where it stood, a wrong entry marks its own
// corner. Nothing ever flashes the whole board.
Item {
    id: cell

    property int value: 0
    property int notes: 0
    property bool given: false
    property bool wrong: false
    property bool selected: false
    property bool peer: false
    property bool sameDigit: false
    property bool highlighted: false
    property real cellSize: width

    // Feedback timings, all named so the board's rhythm is read in one place.
    readonly property int tintMs: 90          // selection and peer shading
    readonly property int settleMs: 110       // accent settle on a placement
    readonly property int retractionMs: 260   // a note fading out of a peer

    signal clicked()

    // Bindings are live from the first frame, so the delegate has to be told
    // when it stops being built: a puzzle loading into 81 cells is not 81
    // placements.
    property bool ready: false
    property int previousNotes: 0
    property int retractedNotes: 0

    Component.onCompleted: {
        cell.previousNotes = cell.notes;
        cell.ready = true;
    }

    // A digit only ever lands in the selected cell, which is also what keeps a
    // wholesale refresh (a new puzzle, a restart) quiet.
    onValueChanged: {
        if (cell.ready && cell.selected && cell.value > 0 && !cell.given)
            settle.restart();
    }

    onNotesChanged: {
        var gone = cell.previousNotes & ~cell.notes;
        cell.previousNotes = cell.notes;
        if (cell.ready && gone !== 0 && cell.value === 0) {
            cell.retractedNotes = gone;
            retraction.restart();
        }
    }

    Rectangle {
        anchors.fill: parent
        color: cell.selected ? theme.alpha(theme.accent, 0.30)
             : cell.highlighted ? theme.alpha(theme.accent, 0.26)
             : cell.sameDigit ? theme.alpha(theme.accent, 0.14)
             : cell.peer ? theme.alpha(theme.foreground, 0.10)
             : "transparent"
        // The selection is an outline as well as a fill, so it survives a
        // theme whose accent barely tints and reads without color alone.
        border.width: cell.selected ? Math.max(2, Math.round(2 * theme.textScale)) : 0
        border.color: theme.accent
        Behavior on color { ColorAnimation { duration: cell.tintMs } }
    }

    Rectangle {
        id: settleTint
        anchors.fill: parent
        color: theme.accent
        opacity: 0

        NumberAnimation {
            id: settle
            target: settleTint
            property: "opacity"
            from: 0.45
            to: 0
            duration: cell.settleMs
            easing.type: Easing.OutCubic
        }
    }

    Text {
        anchors.centerIn: parent
        visible: cell.value > 0
        text: cell.value > 0 ? cell.value.toString() : ""
        // Weight and color both separate a given from an entry.
        color: cell.wrong ? theme.red : cell.given ? theme.foreground : theme.accent
        font.pixelSize: Math.max(1, Math.round(cell.cellSize * 0.6))
        font.bold: cell.given
    }

    // The conflict marker: red ink says which digit is wrong, the dot says so
    // again without relying on the color.
    Rectangle {
        visible: cell.wrong
        width: Math.max(3, Math.round(cell.cellSize * 0.11))
        height: width
        radius: width / 2
        color: theme.red
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: Math.round(cell.cellSize * 0.08)
    }

    NoteGrid {
        anchors.fill: parent
        anchors.margins: Math.round(cell.cellSize * 0.06)
        cellSize: cell.cellSize
        mask: cell.notes
        visible: cell.value === 0 && cell.notes !== 0
    }

    // The marks a placement elsewhere just took away, held for a moment where
    // they stood so the retraction is something you see rather than notice.
    NoteGrid {
        id: ghost
        anchors.fill: parent
        anchors.margins: Math.round(cell.cellSize * 0.06)
        cellSize: cell.cellSize
        mask: cell.retractedNotes
        opacity: 0
        visible: opacity > 0

        NumberAnimation {
            id: retraction
            target: ghost
            property: "opacity"
            from: 0.7
            to: 0
            duration: cell.retractionMs
            easing.type: Easing.OutCubic
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: cell.clicked()
    }
}
