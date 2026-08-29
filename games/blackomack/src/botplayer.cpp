#include "botplayer.h"

#include "basicstrategy.h"

using BlackjackRules::Action;

namespace {
QString band(double v, const char *low, const char *mid, const char *high) {
    return QString::fromUtf8(v < 1.0 / 3 ? low : v < 2.0 / 3 ? mid : high);
}
}

QString BotPersonality::skillLabel() const {
    return band(skill, "rookie", "regular", "pro");
}

QString BotPersonality::aggressionLabel() const {
    return band(aggression, "timid", "steady", "bold");
}

// How the bot bets, then how well it plays: "bold pro", "timid rookie".
QString BotPersonality::label() const {
    return aggressionLabel() + QLatin1Char(' ') + skillLabel();
}

const QStringList &BotPersonality::names() {
    static const QStringList list = {
        QStringLiteral("Zed"), QStringLiteral("Mona"), QStringLiteral("Bucky"), QStringLiteral("Ivy"),
        QStringLiteral("Rex"), QStringLiteral("Lola"), QStringLiteral("Gus"), QStringLiteral("Pip"),
        QStringLiteral("Vera"), QStringLiteral("Otto"), QStringLiteral("Nina"), QStringLiteral("Sal"),
        QStringLiteral("Dot"), QStringLiteral("Hank"), QStringLiteral("Cleo"), QStringLiteral("Moe"),
        QStringLiteral("Tilly"), QStringLiteral("Rufus"), QStringLiteral("Bea"), QStringLiteral("Ace"),
    };
    return list;
}

BotPersonality BotPersonality::roll(const QStringList &taken, QRandomGenerator &rng) {
    QStringList available;
    for (const QString &n : names())
        if (!taken.contains(n))
            available.append(n);
    if (available.isEmpty())
        available = names();
    BotPersonality p;
    p.name = available.at(rng.bounded(available.size()));
    p.skill = rng.generateDouble();
    p.aggression = rng.generateDouble();
    p.seed = rng.generate() | 1;   // never 0, which would mean "random" elsewhere
    return p;
}

BotPlayer::BotPlayer(const BotPersonality &personality, quint32 salt)
    : m_personality(personality), m_rng(personality.seed ^ salt) {}

int BotPlayer::chooseBet(int bankroll) {
    // Timid bots risk ~2% of their stack per hand, bold ones up to 25%.
    const double fraction = 0.02 + 0.23 * m_personality.aggression;
    return BlackjackRules::clampBet(int(bankroll * fraction), bankroll);
}

Action BotPlayer::decide(const Hand &hand, const Card &dealerUp, bool canDouble, bool canSplit) {
    Action strategy = BasicStrategy::decide(hand, dealerUp, canDouble, canSplit);
    if (m_rng.generateDouble() >= m_personality.skill)
        return deviate(hand, dealerUp, strategy);
    // Aggressive bots occasionally double a hard 9-11 the book says to hit.
    if (strategy == Action::Hit && canDouble && !hand.isSoft() && hand.total() >= 9 && hand.total() <= 11
        && m_rng.generateDouble() < m_personality.aggression * 0.3)
        return Action::Double;
    return strategy;
}

Action BotPlayer::deviate(const Hand &hand, const Card &dealerUp, Action strategy) {
    const int total = hand.total();
    if (hand.isSoft())
        return total >= 17 ? Action::Stand : Action::Hit;   // ignores soft doubling entirely
    if (total >= 12 && total <= 16 && dealerUp.value() <= 6 && !dealerUp.isAce())
        return Action::Hit;                                 // the classic "hit a stiff vs a bust card"
    if (strategy == Action::Double || strategy == Action::Split)
        return Action::Hit;                                 // too timid to put more money down
    if (total >= 17)
        return Action::Stand;
    // Gut feeling on the rest: bold bots chase 17, timid ones freeze at 12.
    return (m_personality.aggression > 0.5 ? total < 17 : total < 12) ? Action::Hit : Action::Stand;
}
