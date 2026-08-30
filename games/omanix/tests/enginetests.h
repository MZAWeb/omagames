#pragma once

#include <QObject>

// Rules of the field, the movers and the game state machine, all driven by
// tick() with fixed seeds and hand-placed scenarios.
class EngineTests : public QObject {
    Q_OBJECT

private slots:
    void fieldStartsAsFrameAroundOpenSea();
    void claimTakesTrailAndBallFreeRegions();
    void claimKeepsEveryRegionWithABall();
    void claimMergesWithExistingGround();
    void ballReflectsOffWalls();
    void ballReflectsOffCorner();
    void ballNeverSqueezesBetweenDiagonalBlockers();
    void chaserCrawlsTheFrameClockwise();
    void chaserFollowsNewlyClaimedGround();
    void chaserReversesAtADeadEnd();
    void chaserBuriedByAClaimWalksBackToTheEdge();
    void tapMovesOneCellAndHoldKeepsMoving();
    void marchingIntoOpenSeaCutsATrailUntilGround();
    void cannotReverseOntoTheTrail();
    void ballOnTrailCostsALifeAndWipesIt();
    void crossingOwnTrailCostsALife();
    void chaserContactCostsALifeAnywhere();
    void levelCompletesAtGoal();
    void bigCutMultipliesTheScore();
    void closeCallPaysABonus();
    void extraLifeEveryTenThousand();
    void gameOverAfterLastLife();
    void restartLevelKeepsScoreAndLives();
    void difficultyParametersRamp();
    void theThreeDifficultiesFormALadder();
    void sameSeedSameEvents();
    void levelStartsWithAnIntroFreeze();
    void respawnLandsWhereChasersMustCrawlFurthest();
    void trailThreatenedWhileABallIsNear();
    void groundRevisionOnlyMovesWhenGroundDoes();
    void twentyThousandTicksOfALongCutStayCheap();
};
