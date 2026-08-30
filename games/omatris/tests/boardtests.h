#pragma once

#include <QObject>

// A piece on the stack: gravity, the drops, the lock delay and its allowance,
// hold, the ghost, a line clearing, and the two ways a run tops out.
class BoardTests : public QObject {
    Q_OBJECT

private slots:
    void gravityFollowsTheGuidelineCurve();
    void softDropIsTwentyTimesGravityAndPaysACell();
    void hardDropPaysTwoACellAndLocksAtOnce();
    void lockDelayResetsOnMoveAndCapsAtTheAllowance();
    void spinningOnTheSpotCannotOutlastTheLockDelay();
    void shiftingAtAWallCannotOutlastTheLockDelay();
    void fallingToANewLowestRowRenewsTheAllowance();
    void holdSwapsOncePerPiece();
    void ghostLandsOnTheStack();
    void lineClearFlashesThenCascades();
    void blockOutEndsTheGame();
    void lockOutEndsTheGame();
};
