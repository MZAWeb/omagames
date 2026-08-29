import QtQuick
import QtQuick.Controls

// Flat, theme-tinted button used across games. `primary` fills with the
// accent color; otherwise it is an outlined ghost button.
Button {
    id: control
    property bool primary: false
    property real fontScale: 1.0

    implicitHeight: 40 * theme.textScale
    padding: 12 * theme.textScale
    font.pixelSize: 15 * theme.textScale * fontScale
    font.bold: primary
    flat: true

    background: Rectangle {
        radius: 8
        color: control.primary
            ? (control.down ? theme.mix(theme.accent, theme.foreground, 0.2) : theme.accent)
            : (control.down ? theme.alpha(theme.foreground, 0.16)
               : control.hovered ? theme.alpha(theme.foreground, 0.08) : "transparent")
        border.width: control.primary ? 0 : 1
        border.color: theme.alpha(theme.foreground, control.enabled ? 0.35 : 0.15)
        opacity: control.enabled ? 1.0 : 0.5
        Behavior on color { ColorAnimation { duration: 90 } }
    }

    contentItem: Text {
        text: control.text
        font: control.font
        color: control.primary ? theme.background : theme.foreground
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
