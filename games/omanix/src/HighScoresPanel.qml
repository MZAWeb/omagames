import OmaGames

// The top ten of every difficulty side by side.
OmaScoresPanel {
    title: qsTr("High scores")
    categories: game.difficulties
    entries: game.highScores
    detailText: function(entry) { return qsTr("L%1").arg(entry.level); }
}
