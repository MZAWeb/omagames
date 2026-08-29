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

bool Hand::canSplit() const {
    return cards.size() == 2 && !fromSplit && cards[0].value() == cards[1].value();
}

bool Hand::isFinished() const {
    return stood || doubled || isBust() || (splitAces() && cards.size() >= 2)
        || total() == 21;
}
