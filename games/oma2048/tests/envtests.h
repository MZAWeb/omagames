#pragma once

#include <QObject>

// Oma2048 as an agent plays it (env/): the spec, the mask and afterstates
// against the real slides, a step's reward and signals, the goal, seeded and
// reseeded copies, and a replay drawn back for a viewer.
class EnvTests : public QObject {
    Q_OBJECT

private slots:
    void theSpecDescribesTheGame();
    void theMaskAndAfterstatesAreTheRealSlides();
    void aStepSlidesSpawnsAndPaysTheScore();
    void theSameSeedAndMovesPlayTheSameGame();
    void aReseededCopyKeepsTheBoardNotTheSpawns();
    void theGoalEndsTheRun();
    void aReplayIsDrawnMoveByMove();
};
