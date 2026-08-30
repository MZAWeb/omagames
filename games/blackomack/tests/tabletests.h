#pragma once

#include <QObject>

// A round at the table: the pitch, the turns, the dealer and the payout
// (games/blackomack/src/table.h).
class TableTests : public QObject {
    Q_OBJECT
private slots:
    void openingDealGoesOutOneCardAtATime();
    void naturalPaysThreeToTwo();
    void dealerPeekEndsRound();
    void doubleTakesExactlyOneCard();
    void splitAcesGetOneCardEach();
    void splitHandsPlayInOrderAndMayDouble();
    void dealerStandsOnSoft17AtTheTable();
    void doubleAndSplitNeedFunds();
    void seatingFollowsTheArcAroundTheTable();
    void fullRoundsWithBotsBalanceTheBooks();
    void brokeBotIsReplacedNextRound();
    void nextRoundOnlyAfterPayout();
};
