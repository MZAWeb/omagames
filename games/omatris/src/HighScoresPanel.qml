import OmaGames

// The top ten of every mode side by side: the score for Marathon and Zen, the
// clock for Sprint.
OmaScoresPanel {
    id: root

    title: qsTr("High scores")
    categories: game.modes
    entries: game.highScores
    categoryField: "mode"
    valueText: function(entry, mode) {
        return mode.goal > 0 ? clock.text(entry.millis) : root.numberText(entry.score);
    }
    detailText: function(entry, mode) {
        return mode.goal > 0 ? root.numberText(entry.score) : qsTr("L%1").arg(entry.level);
    }

    TimeFormat { id: clock }
}
