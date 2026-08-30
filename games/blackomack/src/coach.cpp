#include "coach.h"

#include "basicstrategy.h"
#include "table.h"

using BlackjackRules::Action;

namespace {

QString actionText(Action action) {
    switch (action) {
    case Action::Hit: return QStringLiteral("Hit");
    case Action::Stand: return QStringLiteral("Stand");
    case Action::Double: return QStringLiteral("Double");
    case Action::Split: return QStringLiteral("Split");
    }
    return QString();
}

// The coach speaks the way a player would: "Pair of 8s against a 6", never
// "pair 8s vs 6". Only eight takes "an" among the dealer's possible up cards.
QString dealerText(const Card &up) {
    if (up.isAce())
        return QStringLiteral("an ace");
    return QStringLiteral("%1 %2")
        .arg(up.value() == 8 ? QStringLiteral("an") : QStringLiteral("a"))
        .arg(up.value());
}

QString handText(const Hand &hand, bool pairAvailable) {
    if (pairAvailable && hand.canSplit()) {
        const Card &card = hand.cards.first();
        return card.isAce() ? QStringLiteral("Pair of aces")
                            : QStringLiteral("Pair of %1s").arg(card.value());
    }
    if (hand.isSoft())
        return QStringLiteral("Soft %1").arg(hand.total());
    return QString::number(hand.total());
}

}

Advice Coach::adviceFor(const Table &table) {
    // Insurance is a losing bet at every count a basic-strategy player knows.
    if (table.waitingForInsurance())
        return {QStringLiteral("No insurance"), QStringLiteral("Dealer shows an ace")};
    if (!table.waitingForHuman())
        return {};
    const int handIndex = table.currentHand();
    if (handIndex < 0 || handIndex >= table.human().hands.size())
        return {};
    const Hand &hand = table.human().hands[handIndex];
    const bool canDouble = table.canAct(table.humanSeat(), Action::Double);
    const bool canSplit = table.canAct(table.humanSeat(), Action::Split);
    const Action action = BasicStrategy::decide(hand, table.dealerUpCard(), canDouble, canSplit);
    QString situation = QStringLiteral("%1 against %2")
        .arg(handText(hand, canSplit), dealerText(table.dealerUpCard()));
    if (table.human().hands.size() > 1)
        situation.prepend(QStringLiteral("Hand %1 of %2 · ")
                              .arg(handIndex + 1)
                              .arg(table.human().hands.size()));
    return {actionText(action), situation};
}
