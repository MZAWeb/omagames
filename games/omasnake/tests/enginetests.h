#pragma once

#include <QObject>

// The snake, the speed ladder and the game state machine, all driven by
// tick() with fixed seeds and hand-placed scenarios.
class EngineTests : public QObject {
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
    void foodNeverLandsOnTheSnake();
    void fillingTheBoardEndsThePerfectGame();
    void bonusFollowsEveryFifthFood();
    void bonusExpiresAfterItsLifetime();
    void bonusEatenScoresItsFlatFifty();
    void speedRampsWithFoodEaten();
    void theThreeDifficultiesFormALadder();
    void sameSeedSameGame();
};
