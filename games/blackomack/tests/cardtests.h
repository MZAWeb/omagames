#pragma once

#include <QObject>

// The cards themselves: what a hand is worth, what it may do, and the shoe
// they come out of (games/blackomack/src/cards.h, hand.h).
class CardTests : public QObject {
    Q_OBJECT
private slots:
    void cardValues();
    void handTotals();
    void blackjackVsSplitTwentyOne();
    void splitAndDoubleEligibility();
    void splitAcesFinishAfterOneCard();
    void shoeHasSixDecks();
    void shoeIsDeterministicPerSeed();
    void shoeReshuffleThreshold();
};
