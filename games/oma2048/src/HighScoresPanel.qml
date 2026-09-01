import OmaGames

// The top ten runs: score, the highest tile reached beside it, and the date.
OmaScoresPanel {
    title: qsTr("High scores")
    categories: [{ id: "classic", label: qsTr("Top runs") }]
    categoryField: "category"
    entries: game.scores
    maxWidth: 420 * theme.textScale
    detailText: function(entry) { return qsTr("tile %1").arg(entry.tile); }
}
