import OmaGames

// The five fastest solves of every level, side by side.
OmaScoresPanel {
    title: qsTr("Best times")
    maxWidth: 660 * theme.textScale
    tableSpacing: 14 * theme.textScale
    rankWidth: 20 * theme.textScale
    categories: game.difficulties
    entries: game.bestTimes
    emptyText: qsTr("Never solved")
    valueMode: "time"
    elideLabels: true
}
