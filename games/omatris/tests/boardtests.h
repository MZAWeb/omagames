#pragma once

#include <QObject>

// A piece on the stack: gravity, the drops at any speed, hold, the ghost, a
// line clearing, and the two ways a run tops out.
class BoardTests : public QObject {
    Q_OBJECT

private slots:
    void gravityFollowsTheGuidelineCurve();
    void softDropIsTwentyTimesGravityAndPaysACell();
    void softDropFollowsTheChosenFactor();
    void instantSoftDropReachesTheFloorWithoutLocking();
    void hardDropPaysTwoACellAndLocksAtOnce();
    void holdSwapsOncePerPiece();
    void ghostLandsOnTheStack();
    void lineClearFlashesThenCascades();
    void blockOutEndsTheGame();
    void lockOutEndsTheGame();
};
