import OmaGames

// The keys Omasnake plays with, in the order they matter; watching a
// replay, the keys that run it.
OmaKeyLegend {
    model: game.replaying ? [
        { key: qsTr("P"), label: game.paused ? qsTr("play") : qsTr("pause") },
        { key: qsTr("1-4"), label: qsTr("speed") },
        { key: qsTr("→"), label: qsTr("next move") },
        { key: qsTr("R"), label: qsTr("watch again") },
        { key: qsTr("Esc"), label: qsTr("leave") },
        { key: qsTr("Ctrl+Q"), label: qsTr("quit") }
    ] : [
        { key: qsTr("←↑↓→"), label: qsTr("or hjkl to turn") },
        { key: qsTr("Space"), label: qsTr("pause") },
        { key: qsTr("R"), label: qsTr("restart") },
        { key: qsTr("Esc"), label: qsTr("leave") },
        { key: qsTr("Ctrl+Q"), label: qsTr("quit") }
    ]
}
