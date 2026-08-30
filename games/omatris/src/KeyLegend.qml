import OmaGames

// The keys Omatris plays with, in the order they matter.
OmaKeyLegend {
    spacing: 12 * theme.textScale
    model: [
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
