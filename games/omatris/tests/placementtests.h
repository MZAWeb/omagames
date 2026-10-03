#pragma once

#include <QObject>

// What an agent plays through: the calls a game is recorded as, copies and
// reseeded copies of a game, and the search for every place a piece can
// rest (src/calls.h, src/placements.h).
class PlacementTests : public QObject {
    Q_OBJECT

private slots:
    void callsRoundTripTheirTokens();
    void callsDoWhatTheKeysDo();
    void aCopyPlaysOnIdentically();
    void reseedingRedrawsOnlyWhatIsHidden();

    void anEmptyBoardHasEveryDropAndNothingElse();
    void everyLandingIsWhereItsCallsLeadIt();
    void aTSlotIsFoundAsASpin();
    void theHeldPieceLandsToo();
    void dropReachesTheColumnItNames();
    void anAfterstateHasItsLinesCleared();
    void aLandingThatLocksOutSaysSo();
};
