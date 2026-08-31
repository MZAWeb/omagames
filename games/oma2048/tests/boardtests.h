#pragma once

#include <QObject>

// The sliding rules on the bare grid: the worked merge examples from the
// README, merge order, the once-per-move limit, tile identity, scoring, and
// what counts as a possible move.
class BoardTests : public QObject {
    Q_OBJECT

private slots:
    void tripleMergesClosestToTheEdge();
    void fourEqualTilesMergeIntoTwoPairs();
    void unequalNeighbourWorkedExamples();
    void mergedTileNeverMergesAgain();
    void everyDirectionSlidesAndMerges();
    void slideKeepsTheTileId();
    void mergeKeepsTheIdOfTheTileSlidOnto();
    void moveThatChangesNothingReportsIt();
    void scoringIsTheValueOfTheCreatedTile();
    void canMoveMatchesTheBoard();
    void emptyCellsAndHighestValue();
};
