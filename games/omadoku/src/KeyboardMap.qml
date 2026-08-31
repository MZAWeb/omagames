import QtQuick

// The keyboard contract, always on screen under the keypad. It never changes
// with the click mode — that is the whole point of showing it as a statement
// rather than as badges on a selector. It does follow the selection, because
// there a plain digit notes as well, and a line that said otherwise would be
// telling the player something untrue.
Text {
    readonly property bool manyCells: game.selectedIndices.length > 1

    text: manyCells ? qsTr("Keys: 1-9 note every selected cell · Ctrl+1-9 highlight")
                    : qsTr("Keys: 1-9 fill · Shift+1-9 note · Ctrl+1-9 highlight")
    color: theme.mix(theme.background, theme.foreground, 0.62)
    font.pixelSize: 11 * theme.textScale
    wrapMode: Text.WordWrap
    maximumLineCount: 2
    elide: Text.ElideRight
}
