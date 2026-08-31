#pragma once

#include <QObject>

// The run around the board: spawning and its odds, scoring, the win shown
// once, game over, every way undo is and is not available, and the JSON the
// bridge persists.
class GameTests : public QObject {
    Q_OBJECT

private slots:
    void newGameStartsWithTwoTiles();
    void moveSpawnsExactlyOneNewTile();
    void rejectedMoveSpawnsNothing();
    void spawnsAreDeterministicFromTheSeed();
    void spawnsAreNineToOneAndSpreadOut();
    void scoreAccumulatesAcrossMoves();
    void winFiresOnceAndKeepGoingContinues();
    void fullBoardWithAPairIsNotOver();
    void fullBoardWithoutPairsIsOver();
    void moveIntoGameOverEndsTheRun();
    void undoIsUnavailableBeforeTheFirstMove();
    void undoRevertsBoardScoreAndSpawn();
    void undoIsUnavailableTwiceInARow();
    void undoRevertsTheWinningMove();
    void jsonRoundTripsTheRun();
    void restoreLeavesNothingToUndo();
    void restoredWinDoesNotReshowTheOverlay();
    void restoreRejectsMalformedState();
};
