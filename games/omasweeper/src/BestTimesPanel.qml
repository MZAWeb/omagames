import OmaGames

// The five fastest wins of every preset, side by side.
OmaScoresPanel {
    title: qsTr("Best times")
    maxWidth: 640 * theme.textScale
    categories: game.presets
    entries: game.bestTimes
    categoryField: "preset"
    emptyText: qsTr("Never swept")
    valueMode: "time"
}
