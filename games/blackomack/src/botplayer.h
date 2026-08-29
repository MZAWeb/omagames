#pragma once

#include <QRandomGenerator>
#include <QString>
#include <QStringList>

#include "blackjackrules.h"
#include "hand.h"

struct BotPersonality {
    QString name;
    double skill = 0.5;        // probability of following basic strategy per decision
    double aggression = 0.5;   // bet sizing and appetite for risky plays
    quint32 seed = 1;          // drives the bot's private generator

    QString skillLabel() const;       // rookie / regular / pro
    QString aggressionLabel() const;  // timid / steady / bold
    QString label() const;            // "timid pro"

    static const QStringList &names();
    // Random personality with a name not in `taken`.
    static BotPersonality roll(const QStringList &taken, QRandomGenerator &rng);
};

// An AI seat: plays basic strategy with probability `skill`, otherwise makes
// the mistakes typical of casual players. The personality is what persists;
// `salt` varies the decision stream per launch so a bot does not replay the
// exact same mistakes every session (tests leave it 0 for determinism).
class BotPlayer {
public:
    BotPlayer() = default;
    explicit BotPlayer(const BotPersonality &personality, quint32 salt = 0);

    const BotPersonality &personality() const { return m_personality; }

    int chooseBet(int bankroll);
    // Basic strategy never insures, so only the shakier players bite — and the
    // bolder they are, the more the side bet tempts them.
    bool takesInsurance();
    BlackjackRules::Action decide(const Hand &hand, const Card &dealerUp, bool canDouble, bool canSplit);

private:
    BlackjackRules::Action deviate(const Hand &hand, const Card &dealerUp, BlackjackRules::Action strategy);

    BotPersonality m_personality;
    QRandomGenerator m_rng;
};
