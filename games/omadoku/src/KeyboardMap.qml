import QtQuick

// The keyboard contract, always on screen under the keypad. It never changes
// with the click mode — that is the whole point of showing it as a statement
// rather than as badges on a selector.
Text {
    text: qsTr("Keys: 1-9 fill · Shift+1-9 note · Ctrl+1-9 highlight")
    color: theme.mix(theme.background, theme.foreground, 0.62)
    font.pixelSize: 11 * theme.textScale
    wrapMode: Text.WordWrap
    maximumLineCount: 2
    elide: Text.ElideRight
}
