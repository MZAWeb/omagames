#pragma once

#include <QObject>

// How long a landed piece has before it locks: the allowance of moves that
// buy more time, and the ways a player might try to stretch it.
class LockDelayTests : public QObject {
    Q_OBJECT

private slots:
    void theDelayCountsRestingTicksAndSpendsResets();
    void lockDelayResetsOnMoveAndCapsAtTheAllowance();
    void spinningOnTheSpotCannotOutlastTheLockDelay();
    void shiftingAtAWallCannotOutlastTheLockDelay();
    void fallingToANewLowestRowRenewsTheAllowance();
    void aSlideToTheWallIsChargedAsOneMove();
};
