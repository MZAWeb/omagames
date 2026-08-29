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

    QString skillLabel() const;       // reckless / average / sharp
    QString aggressionLabel() const;  // cautious / steady / wild
    QString label() const;            // "cautious · sharp"

    static const QStringList &names();
    // Random personality with a name not in `taken`.
    static BotPersonality roll(const QStringList &taken, QRandomGenerator &rng);
};

// An AI seat: plays basic strategy with probability `skill`, otherwise makes
// the mistakes typical of casual players. All randomness comes from the
// personality seed so a bot behaves the same way across launches.
class BotPlayer {
public:
    BotPlayer() = default;
    explicit BotPlayer(const BotPersonality &personality);

    const BotPersonality &personality() const { return m_personality; }

    int chooseBet(int bankroll);
    BlackjackRules::Action decide(const Hand &hand, const Card &dealerUp, bool canDouble, bool canSplit);

private:
    BlackjackRules::Action deviate(const Hand &hand, const Card &dealerUp, BlackjackRules::Action strategy);

    BotPersonality m_personality;
    QRandomGenerator m_rng;
};
