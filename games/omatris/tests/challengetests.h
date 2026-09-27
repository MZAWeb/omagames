#pragma once

#include <QObject>

// Challenge: the mess it deals, where the dealt rows go as lines clear, and
// the run ending with the last of them.
class ChallengeTests : public QObject {
    Q_OBJECT

private slots:
    void dealtStackLooksPlayed();
    void theSameSeedDealsTheSameMess();
    void clearedRowsLeaveAndTheRestFall();
    void theStackCountsFlashingRowsAsGone();
    void clearingEveryDealtRowFinishesAtZenPace();
    void aClearAboveTheStackDoesNotCount();
};
