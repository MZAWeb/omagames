#pragma once

#include <QObject>

// The rules: the SRS tables, the board, the randomiser and the state machine,
// all driven by tick() with fixed seeds and hand-built scenarios.
class EngineTests : public QObject {
    Q_OBJECT

private slots:
    void spawnOrientationsMatchTheGuideline();
    void rotationStatesCycleBothWays();
    void kickTablesMatchTheGuideline();
    void iPieceKicksOffAWall();
    void tPieceKicksAgainstBothWalls();
    void oPieceNeverMovesWhenTurned();
    void sevenBagDealsEveryPieceOncePerSeven();
    void sameSeedPlaysTheSameGame();
    void gravityFollowsTheGuidelineCurve();
    void softDropIsTwentyTimesGravityAndPaysACell();
    void hardDropPaysTwoACellAndLocksAtOnce();
    void lockDelayResetsOnMoveAndCapsAtFifteen();
    void holdSwapsOncePerPiece();
    void ghostLandsOnTheStack();
    void lineClearFlashesThenCascades();
    void scoringPaysTheGuidelineTable();
    void backToBackAndComboStack();
    void tSpinTripleScoresAsAFullSpin();
    void tSpinMiniIsToldFromAFullOne();
    void levelRisesEveryTenLines();
    void sprintFinishesAtFortyLinesAndSprintZenNeverRamp();
    void blockOutEndsTheGame();
    void lockOutEndsTheGame();
};
