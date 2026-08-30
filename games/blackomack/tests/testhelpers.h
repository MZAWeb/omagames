#pragma once

#include <QtTest>

#include "blackjackgame.h"
#include "botplayer.h"
#include "cards.h"
#include "hand.h"
#include "table.h"

// The fixtures every suite builds its rounds from: cards and hands written as
// ranks, and the two loops that drive a table the way the bridge does.

inline Card c(int rank, Suit suit = Suit::Spades) { return {rank, suit}; }

inline Hand hand(std::initializer_list<int> ranks, int bet = 10) {
    Hand h;
    for (int r : ranks)
        h.cards.append(c(r));
    h.bet = bet;
    return h;
}

inline QVector<Card> cards(std::initializer_list<int> ranks) {
    QVector<Card> v;
    for (int r : ranks)
        v.append(c(r));
    return v;
}

// Pitches the opening cards one step at a time, the way the bridge does.
inline QVector<TableEvent> dealOut(Table &t) {
    QVector<TableEvent> events;
    while (t.phase() == Table::Phase::Dealing)
        events += t.advance();
    return events;
}

// Runs automatic steps until the human must act or the round is settled.
inline void autoplay(Table &t) {
    while (!t.waitingForHuman() && !t.roundOver() && t.phase() != Table::Phase::Betting) {
        if (t.waitingForInsurance())
            t.declineInsurance();   // the book never insures
        else
            t.advance();
    }
}

inline BotPersonality perfect(const QString &name, quint32 seed = 1) { return {name, 1.0, 0.5, seed}; }

// Plays the human's hands by the book until the round is settled.
inline void playRound(BlackjackGame &g) {
    g.dealRound();
    while (!g.roundOver()) {
        if (g.canInsure())
            g.declineInsurance();
        else if (g.canStand())
            g.stand();
        else
            QVERIFY(g.waitingForHuman());
    }
}
