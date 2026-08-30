import QtQuick
import QtQuick.Layouts
import OmaGames

// The title of one group of settings, with a rule out to the key that changes
// the group when it has one. Two of these are what keep the walls rule and the
// speed from reading as a single list of buttons.
RowLayout {
    id: root

    property string text: ""
    property string hint: ""

    spacing: 8 * theme.textScale

    Text {
        text: root.text
        color: theme.mix(theme.background, theme.foreground, 0.7)
        font.pixelSize: 12 * theme.textScale
        font.bold: true
    }
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        color: theme.alpha(theme.foreground, 0.15)
    }
    OmaKeyHint {
        key: root.hint
    }
}
