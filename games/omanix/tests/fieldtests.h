#pragma once

#include <QObject>

// The field itself: the frame it starts as, and what closing a trail claims.
class FieldTests : public QObject {
    Q_OBJECT

private slots:
    void fieldStartsAsFrameAroundOpenSea();
    void claimTakesTrailAndBallFreeRegions();
    void claimKeepsEveryRegionWithABall();
    void claimMergesWithExistingGround();
};
