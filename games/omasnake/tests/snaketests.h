#pragma once

#include <QObject>

// The snake under the player's hand: where it starts, the beats that hold it
// still, how turns are taken, how it grows, and every way a run ends.
class SnakeTests : public QObject {
    Q_OBJECT

private slots:
    void snakeStartsCentredAndKeepsItsHeading();
    void readyBeatHoldsTheSnakeStill();
    void pauseHoldsTheSnakeStill();
    void turnsAreTakenOneMoveAtATime();
    void reversingIsIgnored();
    void thirdQueuedTurnIsDropped();
    void eatingGrowsAndScores();
    void lengthMultiplierClimbsWithTheSnake();
    void wallEndsTheGameWithNoGraceMove();
    void wrapModeCarriesTheSnakeAcross();
    void runningIntoItselfEndsTheGame();
    void followingItsOwnTailIsLegal();
    void gameOverFreezesEverything();
};
