import QtQuick

// One tile. Its color climbs a theme.mix ramp from near the background at 2
// up to the accent at 2048 and beyond, so the board follows the desktop; the
// ink flips to the background color once the fill gets close to the accent,
// the same pairing as a primary OmaButton.
Rectangle {
    id: tile

    required property int value
    required property int row
    required property int col
    property real cell: 0
    property real gap: 0

    // 2 → 0, 2048 → 1, clamped above.
    readonly property real ramp: Math.min(1, (Math.log2(value) - 1) / 10)

    x: gap + col * (cell + gap)
    y: gap + row * (cell + gap)
    width: cell
    height: cell
    radius: cell * 0.08
    color: theme.mix(theme.mix(theme.background, theme.foreground, 0.12), theme.accent, ramp)

    Behavior on x { NumberAnimation { duration: 110; easing.type: Easing.OutCubic } }
    Behavior on y { NumberAnimation { duration: 110; easing.type: Easing.OutCubic } }

    // A spawned tile fades and grows in; the initial board does the same,
    // which doubles as the new-game flourish.
    NumberAnimation on opacity { from: 0; to: 1; duration: 160 }
    NumberAnimation on scale { from: 0.55; to: 1; duration: 160; easing.type: Easing.OutBack }

    // A merge bumps the value of the surviving tile, and the pop marks it.
    SequentialAnimation {
        id: pop
        NumberAnimation { target: tile; property: "scale"; to: 1.15; duration: 70; easing.type: Easing.OutQuad }
        NumberAnimation { target: tile; property: "scale"; to: 1.0; duration: 90; easing.type: Easing.InQuad }
    }

    property bool live: false
    Component.onCompleted: live = true
    onValueChanged: if (live) pop.restart()

    Text {
        anchors.centerIn: parent
        width: tile.cell * 0.9
        height: tile.cell * 0.9
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        // No thousands separator: the tile reads "2048", not "2,048".
        text: tile.value.toString()
        color: tile.ramp < 0.5 ? theme.foreground : theme.background
        font.pixelSize: Math.max(8, Math.round(tile.cell * 0.42))
        minimumPixelSize: 8
        fontSizeMode: Text.Fit
        font.bold: true
    }
}
