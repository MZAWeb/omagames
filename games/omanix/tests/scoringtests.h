#pragma once

#include <QObject>

// What a run is worth and how it ends: claims, bonuses, lives, levels, the
// difficulty ladder and the determinism the seed promises.
class ScoringTests : public QObject {
    Q_OBJECT

private slots:
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
};
