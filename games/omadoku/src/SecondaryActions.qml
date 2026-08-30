import QtQuick
import QtQuick.Layouts
import OmaGames

// What you do to the puzzle rather than to a cell. The order never changes and
// only the number of columns does, so the rail and the bottom sheet hold the
// same four actions in the same reading order.
GridLayout {
    id: actions

    signal leaveRequested()

    columns: 2
    columnSpacing: 8 * theme.textScale
    rowSpacing: 8 * theme.textScale

    OmaHintButton {
        Layout.fillWidth: true
        text: qsTr("Erase")
        hint: qsTr("Bksp")
        onClicked: game.erase()
    }
    OmaHintButton {
        Layout.fillWidth: true
        text: qsTr("Undo")
        hint: qsTr("Ctrl+Z")
        enabled: game.canUndo
        onClicked: game.undo()
    }
    OmaHintButton {
        Layout.fillWidth: true
        text: qsTr("Restart")
        hint: qsTr("R")
        onClicked: game.requestRestart()
    }
    OmaHintButton {
        Layout.fillWidth: true
        text: qsTr("New game")
        hint: qsTr("Esc")
        onClicked: actions.leaveRequested()
    }
}
