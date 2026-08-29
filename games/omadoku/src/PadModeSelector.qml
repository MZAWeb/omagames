import QtQuick
import QtQuick.Layouts
import OmaGames
import Omadoku

// Picks what a click on the keypad does. The three modes mirror the number
// row's plain, Shift and Ctrl meanings, which the badges spell out.
ColumnLayout {
    id: root

    spacing: 5 * theme.textScale

    readonly property var modes: [
        {mode: SudokuGame.Highlight, label: qsTr("Highlight"), hint: qsTr("1-9")},
        {mode: SudokuGame.Note, label: qsTr("Note"), hint: qsTr("Shift+1-9")},
        {mode: SudokuGame.Fill, label: qsTr("Fill"), hint: qsTr("Ctrl+1-9")},
    ]

    RowLayout {
        Layout.fillWidth: true
        spacing: 6 * theme.textScale

        Text {
            text: qsTr("Click mode")
            color: theme.mix(theme.background, theme.foreground, 0.55)
            font.pixelSize: 12 * theme.textScale
        }
        OmaKeyHint { key: qsTr("N") }
        Item { Layout.fillWidth: true }
    }

    Repeater {
        model: root.modes

        HintButton {
            required property var modelData
            Layout.fillWidth: true
            text: modelData.label
            keyHint: modelData.hint
            primary: game.padMode === modelData.mode
            onClicked: game.padMode = modelData.mode
        }
    }
}
