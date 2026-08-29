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
    int insuranceBet = 0;       // side bet against a dealer ace, 0 when declined
    int insuranceReturned = 0;  // stake + winnings paid back on a dealer natural

    QString name() const;
    bool broke() const { return bankroll < BlackjackRules::kMinBet; }
};

// Something that happened at the table; the UI logs the text and can animate
// by type. `seat`/`hand` are -1 when not about a specific hand, except on a
// `Dealt` event, where `seat` is `Table::kDealerSeat` for the dealer's card.
struct TableEvent {
    enum Type {
        Shuffled, BetPlaced, Dealt, DealerBlackjack, PlayerAction, HumanTurn,
        DealerReveal, DealerCard, DealerStand, HandResolved, BotLeft, BotJoined,
        Insurance, InsuranceResolved
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
    enum class Phase { Betting, Dealing, Insurance, PlayerTurns, DealerTurn, Payout };
    using Action = BlackjackRules::Action;
    static constexpr int kDealerSeat = -1;

    explicit Table(quint32 shoeSeed = 0);

    Phase phase() const { return m_phase; }
    const QVector<Seat> &seats() const { return m_seats; }
    int humanSeat() const { return m_humanSeat; }
    const Seat &human() const { return m_seats[m_humanSeat]; }
    int botCount() const { return m_seats.size() - 1; }
    const Hand &dealer() const { return m_dealer; }
    int shoeRemaining() const { return m_shoe.remaining(); }
    bool holeHidden() const { return m_holeHidden; }
    Card dealerUpCard() const { return m_dealer.cards.value(0); }
    int currentSeat() const { return m_phase == Phase::PlayerTurns ? m_seat : -1; }
    int currentHand() const { return m_phase == Phase::PlayerTurns ? m_hand : -1; }
    bool waitingForHuman() const { return m_phase == Phase::PlayerTurns && m_seats[m_seat].isHuman; }
    // The seat the insurance cursor is on may still take the side bet; while
    // it is the human's, the table waits for takeInsurance/declineInsurance.
    bool canInsure(int seat) const { return m_phase == Phase::Insurance && seat == m_insureSeat; }
    bool waitingForInsurance() const { return canInsure(m_humanSeat); }
    int insuranceCost(int seat) const;
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
    // After the bets the opening deal, the bots and the dealer all run through
    // `advance()`, one card or one decision at a time.
    QVector<TableEvent> placeBets(int humanBet);
    bool canAct(int seat, Action action) const;
    QVector<TableEvent> act(Action action);     // the human's current hand
    QVector<TableEvent> takeInsurance();        // the human's insurance decision
    QVector<TableEvent> declineInsurance();
    QVector<TableEvent> advance();              // next automatic step
    // `rng` rolls replacements for broke bots and salts their decision streams.
    QVector<TableEvent> nextRound(QRandomGenerator &rng);

private:
    Card draw();
    void rebalanceSeats();
    QVector<int> dealOrder() const;
    QVector<TableEvent> dealStep();
    QVector<TableEvent> openTurns();
    QVector<TableEvent> peekAndOpen();
    bool findInsuranceSeat();
    QVector<TableEvent> insuranceStep();
    QVector<TableEvent> applyInsurance(bool take);
    QVector<TableEvent> settleInsurance();
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
    int m_dealt = 0;
    Phase m_phase = Phase::Betting;
    int m_seat = 0;
    int m_hand = 0;
    int m_insureSeat = 0;
    bool m_resolved = false;
};
