#pragma once

#include <QObject>

// The two kinds of mover: balls bouncing off whatever they meet, and chasers
// crawling the boundary of the claimed ground.
class MoverTests : public QObject {
    Q_OBJECT

private slots:
    void ballReflectsOffWalls();
    void ballReflectsOffCorner();
    void ballNeverSqueezesBetweenDiagonalBlockers();
    void chaserCrawlsTheFrameClockwise();
    void chaserFollowsNewlyClaimedGround();
    void chaserReversesAtADeadEnd();
    void chaserBuriedByAClaimWalksBackToTheEdge();
    void chaserPicksJunctionsFromTheSeed();
    void chaserLeavesAClosedPocketForAnotherRegion();
    void chaserForgetsItsTrackWhenAClaimMovesTheBoundary();
};
