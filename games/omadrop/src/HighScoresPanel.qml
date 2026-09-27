import OmaGames

OmaScoresPanel {
    title: qsTr("High scores")
    categories: [{ id: "standard", label: qsTr("Top runs") }]
    categoryField: "category"
    entries: game.highScores
    maxWidth: 440 * theme.textScale
    detailText: function(entry) { return qsTr("%1 shots").arg(entry.shots); }
}
