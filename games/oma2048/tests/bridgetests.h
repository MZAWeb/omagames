#pragma once

#include <QObject>
#include <QString>

// The Oma2048Game bridge: the tile model's incremental updates, the property
// diffs QML sees, the score kept exactly once at game over, and the state/v1
// run across relaunches.
class BridgeTests : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void startsANewGameWhenNothingIsSaved();
    void plainMoveUpdatesRowsWithoutAReset();
    void mergeRemovesTheMergedTileAndAppendsTheSpawn();
    void winningShowsTheOverlayOnceAndKeepGoingClearsIt();
    void gameOverRecordsTheScoreOnce();
    void aRunBelowTheTableGetsNoRank();
    void undoRevertsThroughTheBridgeAndSyncsTheModel();
    void stateRoundTripsThroughAFreshBridge();
    void restoreLeavesUndoUnavailable();
    void newGameClearsTheRun();
    void windowGeometryRoundTrips();

private:
    QString m_settingsDir;
};
