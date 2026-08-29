import QtQuick

// Small keycap badge ("H", "Enter", "Ctrl+N") that tells keyboard-first users
// which key triggers a control. Set `onPrimary` when drawn over an accent
// fill; `active: false` greys it out along with a disabled action. The cap
// always grows to its text; `fontPixelSize` is for callers whose slot cannot
// grow, so they shrink the label rather than have it clipped.
Rectangle {
    id: hint
    property string key: ""
    property bool active: true
    property bool onPrimary: false
    property real fontPixelSize: 11 * theme.textScale

    visible: key !== ""
    implicitWidth: label.implicitWidth + 8 * theme.textScale
    implicitHeight: label.implicitHeight + 3 * theme.textScale
    radius: 4
    color: onPrimary ? theme.alpha(theme.background, 0.22) : theme.alpha(theme.foreground, 0.12)
    border.width: 1
    border.color: onPrimary ? theme.alpha(theme.background, 0.35) : theme.alpha(theme.foreground, 0.25)
    opacity: active ? 1.0 : 0.45

    Text {
        id: label
        anchors.centerIn: parent
        text: hint.key
        color: hint.onPrimary ? theme.background : theme.foreground
        font.pixelSize: hint.fontPixelSize
        font.bold: true
    }
}
