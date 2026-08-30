import QtQuick
import QtQuick.Layouts

// The rail folded under the board, for windows too narrow — or text too large —
// to carry one beside it. Same controls in the same reading order, left to
// right and then down, so the switch costs no relearning.
RowLayout {
    id: sheet

    // True when the sheet is too narrow to hold the four actions in one row.
    property bool tight: false
    signal leaveRequested()

    spacing: 12 * theme.textScale

    // The keypad is sized from the sheet's height, which the board screen
    // hands down as a constant: measuring it from the sheet's own contents
    // instead would be a loop, since the board is sized from what is left.
    DigitPad {
        Layout.alignment: Qt.AlignVCenter
        keySize: Math.max(18, Math.min(44 * theme.textScale, sheet.height / 3.24,
                                       sheet.width * 0.3 / 3.24))
        onDigitPressed: function(digit) { game.clickDigit(digit); }
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignVCenter
        spacing: 8 * theme.textScale

        KeyboardMap { Layout.fillWidth: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8 * theme.textScale

            ClickModeSelector { Layout.fillWidth: true }
            ValidateToggle { Layout.alignment: Qt.AlignBottom; compact: true }
        }

        SecondaryActions {
            Layout.fillWidth: true
            columns: sheet.tight ? 2 : 4
            onLeaveRequested: sheet.leaveRequested()
        }

        KeyReminder { Layout.fillWidth: true }
    }
}
