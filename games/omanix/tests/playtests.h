#pragma once

#include <QObject>

// The marker under the player's hand: how it moves, the trail it leaves, and
// every way a life is lost.
class PlayTests : public QObject {
    Q_OBJECT

private slots:
    void tapMovesOneCellAndHoldKeepsMoving();
    void marchingIntoOpenSeaCutsATrailUntilGround();
    void cannotReverseOntoTheTrail();
    void ballOnTrailCostsALifeAndWipesIt();
    void crossingOwnTrailCostsALife();
    void chaserContactCostsALifeAnywhere();
};
