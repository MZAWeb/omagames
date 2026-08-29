#include "hand.h"

namespace {
int hardTotal(const QVector<Card> &cards) {
    int sum = 0;
    for (const Card &c : cards)
        sum += c.value();
    return sum;
}
bool hasAce(const QVector<Card> &cards) {
    for (const Card &c : cards)
        if (c.isAce())
            return true;
    return false;
}
}

int Hand::total() const {
    const int hard = hardTotal(cards);
    return isSoft() ? hard + 10 : hard;
}

bool Hand::isSoft() const {
    return hasAce(cards) && hardTotal(cards) + 10 <= 21;
}

bool Hand::isBlackjack() const {
    return cards.size() == 2 && total() == 21 && !fromSplit;
}

// A pair can be split again — the table caps how many hands a seat may end up
// with — except for split aces, which take their one card and stand.
bool Hand::canSplit() const {
    return cards.size() == 2 && cards[0].value() == cards[1].value() && !splitAces();
}

bool Hand::isFinished() const {
    return stood || doubled || isBust() || (splitAces() && cards.size() >= 2)
        || total() == 21;
}
