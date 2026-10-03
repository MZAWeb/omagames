#pragma once

#include <QObject>

// Omasnake as an agent plays it (env/omasnakeenv.h): a step per move, both
// ways to steer, what is observed and rewarded, the replay it keeps, and
// what it must never touch.
class EnvTests : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    void theSpecDescribesBothActionSpaces();
    void aStepIsOneMove();
    void relativeTurnsAreAgainstTheHeading();
    void absoluteMasksTheWayBack();
    void eatingRewardsAndGrows();
    void theWallEndsAClassicRunAndAWrapRunCrossesIt();
    void theGridShowsTheSnakeAndItsFood();
    void theSameSeedAndTurnsPlayTheSameGame();
    void theReplayIsTheGame();
    void aReseededCloneHidesWhereFoodWillLand();
    void theEnvLeavesSettingsAlone();
    void theAbiPlaysOmasnake();
};
