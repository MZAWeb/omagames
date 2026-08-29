import QtQuick
import OmaGames

// OmaButton with its keyboard shortcut shown as a keycap after the label.
// `showHint: false` hides the keycap for a key that is not live right now.
OmaButton {
    id: control
    property string hint: ""
    property bool showHint: true

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
            key: control.showHint ? control.hint : ""
            active: control.enabled
            onPrimary: control.primary
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
