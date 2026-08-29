#pragma once

#include <QRandomGenerator>
#include <QString>
#include <QStringList>
#include <QVector>

#include "blackjackrules.h"
#include "botplayer.h"
#include "cards.h"
#include "hand.h"

struct Seat {
    bool isHuman = false;
    BotPlayer bot;          // unused for the human seat
    int bankroll = BlackjackRules::kStartingBankroll;
    QVector<Hand> hands;    // empty while betting or when sitting out

    QString name() const;
    bool broke() const { return bankroll < BlackjackRules::kMinBet; }
};

// Something that happened at the table; the UI logs the text and can animate
// by type. `seat`/`hand` are -1 when not about a specific hand.
struct TableEvent {
    enum Type {
        Shuffled, BetPlaced, Dealt, DealerBlackjack, PlayerAction, HumanTurn,
        DealerReveal, DealerCard, DealerStand, HandResolved, BotLeft, BotJoined
    };
    Type type;
    QString text;
    int seat = -1;
    int hand = -1;
};

// One blackjack round as a pure step-wise state machine: no timers, no Qt
// GUI. The bridge calls `advance()` at its own pace for bot and dealer moves
// and `act()` when the human plays.
class Table {
public:
    enum class Phase { Betting, Dealing, PlayerTurns, DealerTurn, Payout };
    using Action = BlackjackRules::Action;

    explicit Table(quint32 shoeSeed = 0);

    Phase phase() const { return m_phase; }
    const QVector<Seat> &seats() const { return m_seats; }
    int humanSeat() const { return m_humanSeat; }
    const Seat &human() const { return m_seats[m_humanSeat]; }
    int botCount() const { return m_seats.size() - 1; }
    const Hand &dealer() const { return m_dealer; }
    bool holeHidden() const { return m_holeHidden; }
    Card dealerUpCard() const { return m_dealer.cards.value(0); }
    int currentSeat() const { return m_phase == Phase::PlayerTurns ? m_seat : -1; }
    int currentHand() const { return m_phase == Phase::PlayerTurns ? m_hand : -1; }
    bool waitingForHuman() const { return m_phase == Phase::PlayerTurns && m_seats[m_seat].isHuman; }
    bool roundOver() const { return m_phase == Phase::Payout && m_resolved; }
    QStringList takenNames() const;

    // Setup — only between rounds (Betting). The human keeps the middle seat.
    void setHumanBankroll(int bankroll);
    bool addBot(const BotPersonality &personality, int bankroll = BlackjackRules::kStartingBankroll,
                quint32 salt = 0);
    bool removeLastBot();
    // Puts cards on top of the shoe, first one drawn first (tests script rounds with it).
    void stackDeck(const QVector<Card> &cards);

    // Round flow. Each step returns the events it produced (empty if refused).
    QVector<TableEvent> placeBets(int humanBet);
    QVector<TableEvent> deal();
    bool canAct(int seat, Action action) const;
    QVector<TableEvent> act(Action action);     // the human's current hand
    QVector<TableEvent> advance();              // next automatic step
    // `rng` rolls replacements for broke bots and salts their decision streams.
    QVector<TableEvent> nextRound(QRandomGenerator &rng);

private:
    Card draw();
    void rebalanceSeats();
    void moveCursor();
    QVector<TableEvent> apply(int seat, Action action);
    QVector<TableEvent> dealerStep();
    QVector<TableEvent> resolve();
    bool anyLiveHand() const;

    Shoe m_shoe;
    QVector<Card> m_stacked;
    QVector<Seat> m_seats;
    int m_humanSeat = 0;
    Hand m_dealer;
    bool m_holeHidden = true;
    Phase m_phase = Phase::Betting;
    int m_seat = 0;
    int m_hand = 0;
    bool m_resolved = false;
};
