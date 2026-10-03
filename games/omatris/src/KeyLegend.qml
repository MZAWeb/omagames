import OmaGames

// The keys Omatris plays with, in the order they matter; watching a
// replay, the keys that run it.
OmaKeyLegend {
    spacing: 12 * theme.textScale
    model: game.replaying ? [
        { key: qsTr("P"), label: game.paused ? qsTr("play") : qsTr("pause") },
        { key: qsTr("1-4"), label: qsTr("speed") },
        { key: qsTr("→"), label: qsTr("next piece") },
        { key: qsTr("R"), label: qsTr("watch again") },
        { key: qsTr("G"), label: game.ghostEnabled ? qsTr("ghost on") : qsTr("ghost off") },
        { key: qsTr("Esc"), label: qsTr("leave") },
        { key: qsTr("Ctrl+Q"), label: qsTr("quit") }
    ] : [
        { key: qsTr("←→"), label: qsTr("move") },
        { key: qsTr("↓"), label: qsTr("soft drop") },
        { key: qsTr("Space"), label: qsTr("hard drop") },
        { key: qsTr("↑"), label: qsTr("or X to rotate") },
        { key: qsTr("Z"), label: qsTr("rotate back") },
        { key: qsTr("C"), label: qsTr("hold") },
        { key: qsTr("G"), label: game.ghostEnabled ? qsTr("ghost on") : qsTr("ghost off") },
        { key: qsTr("P"), label: qsTr("pause") },
        { key: qsTr("R"), label: qsTr("restart") },
        { key: qsTr("Esc"), label: qsTr("leave") },
        { key: qsTr("Ctrl+Q"), label: qsTr("quit") }
    ]
}
