import QtQuick

// The foot of the rail: the keys no control on screen can wear, in one line.
// It wraps to a second line before it elides, so a large text scale costs a
// line rather than the sentence.
Text {
    text: qsTr("Arrows / hjkl move · N cycles the click mode · Esc backs out")
    color: theme.mix(theme.background, theme.foreground, 0.5)
    font.pixelSize: 11 * theme.textScale
    wrapMode: Text.WordWrap
    maximumLineCount: 2
    elide: Text.ElideRight
}
