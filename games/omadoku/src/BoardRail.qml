import QtQuick
import QtQuick.Layouts

// The rail beside the board: the digits, the keyboard contract, what a click
// on them does, validation, the secondary actions and a one-line key reminder —
// always in that order, so nothing a hand has learned moves when the puzzle or
// the window changes.
ColumnLayout {
    id: rail

    signal leaveRequested()

    spacing: 10 * theme.textScale

    // What is left for the keypad once everything under it has taken its
    // share. A key plus its gap is 1.08 keys wide, hence the 3.24: expressing
    // it that way keeps the keypad's own spacing out of the binding and avoids
    // a loop. The rail's height is anchored, never asked of its children.
    readonly property real keyBudget: height - keys.implicitHeight - modes.implicitHeight
        - validate.implicitHeight - actions.implicitHeight - reminder.implicitHeight - 6 * spacing

    DigitPad {
        Layout.alignment: Qt.AlignHCenter
        keySize: Math.max(18, Math.min(rail.keyBudget / 3.24, rail.width / 3.24))
        onDigitPressed: function(digit) { game.clickDigit(digit); }
    }

    KeyboardMap {
        id: keys
        Layout.fillWidth: true
    }

    ClickModeSelector {
        id: modes
        Layout.fillWidth: true
    }

    ValidateToggle {
        id: validate
        Layout.fillWidth: true
    }

    SecondaryActions {
        id: actions
        Layout.fillWidth: true
        columns: 2
        onLeaveRequested: rail.leaveRequested()
    }

    Item { Layout.fillHeight: true }

    KeyReminder {
        id: reminder
        Layout.fillWidth: true
    }
}
