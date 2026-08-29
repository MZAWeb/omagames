import QtQuick
import QtQuick.Layouts
import OmaGames

// Spells out what the number row does, since the same nine keys mean three
// different things depending on the modifier.
RowLayout {
    spacing: 6 * theme.textScale

    component Entry: RowLayout {
        property alias key: badge.key
        property alias label: text.text
        spacing: 4 * theme.textScale
        OmaKeyHint { id: badge }
        Text {
            id: text
            color: theme.mix(theme.background, theme.foreground, 0.55)
            font.pixelSize: 12 * theme.textScale
        }
    }

    Entry { key: qsTr("1-9"); label: qsTr("highlight") }
    Entry { key: qsTr("Shift+1-9"); label: qsTr("note"); Layout.leftMargin: 8 * theme.textScale }
    Entry { key: qsTr("Ctrl+1-9"); label: qsTr("enter"); Layout.leftMargin: 8 * theme.textScale }
}
