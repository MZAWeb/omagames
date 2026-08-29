import QtQuick
import OmaGames

// OmaButton with its keyboard shortcut shown as a keycap after the label.
OmaButton {
    id: control
    property string hint: ""

    contentItem: Row {
        spacing: 8 * theme.textScale
        Text {
            text: control.text
            font: control.font
            color: control.primary ? theme.background : theme.foreground
            verticalAlignment: Text.AlignVCenter
            height: parent.height
        }
        OmaKeyHint {
            key: control.hint
            active: control.enabled
            onPrimary: control.primary
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
