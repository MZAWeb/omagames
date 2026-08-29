#pragma once

#include <QVector>

#include "cards.h"

struct Hand {
    QVector<Card> cards;
    int bet = 0;
    bool doubled = false;
    bool fromSplit = false;
    bool stood = false;

    int total() const;          // best total: one ace counts 11 unless that busts
    bool isSoft() const;        // an ace is currently counted as 11
    bool isBust() const { return total() > 21; }
    bool isBlackjack() const;   // natural: two cards, 21, not after a split
    bool canSplit() const;      // two cards of equal value, not already split
    bool canDouble() const { return cards.size() == 2 && !stood; }
    // Split aces get exactly one card and stand; also true after a double or bust.
    bool isFinished() const;
    bool splitAces() const { return fromSplit && !cards.isEmpty() && cards.first().isAce(); }
};
