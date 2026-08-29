#include "table.h"

using namespace BlackjackRules;

QString Seat::name() const {
    return isHuman ? QStringLiteral("You") : bot.personality().name;
}

Table::Table(quint32 shoeSeed) : m_shoe(kDecks, shoeSeed) {
    Seat human;
    human.isHuman = true;
    m_seats.append(human);
}

QStringList Table::takenNames() const {
    QStringList names;
    for (const Seat &s : m_seats)
        if (!s.isHuman)
            names.append(s.bot.personality().name);
    return names;
}

void Table::setHumanBankroll(int bankroll) {
    m_seats[m_humanSeat].bankroll = bankroll;
}

bool Table::addBot(const BotPersonality &personality, int bankroll, quint32 salt) {
    if (m_phase != Phase::Betting || botCount() >= 5)
        return false;
    Seat seat;
    seat.bot = BotPlayer(personality, salt);
    seat.bankroll = bankroll;
    m_seats.append(seat);
    rebalanceSeats();
    return true;
}

bool Table::removeLastBot() {
    if (m_phase != Phase::Betting || botCount() == 0)
        return false;
    for (int i = m_seats.size() - 1; i >= 0; --i) {
        if (!m_seats[i].isHuman) {
            m_seats.removeAt(i);
            break;
        }
    }
    rebalanceSeats();
    return true;
}

// Keeps the human in the middle of the table so bots act both before and after.
void Table::rebalanceSeats() {
    const Seat human = m_seats.takeAt(m_humanSeat);
    m_humanSeat = m_seats.size() / 2;
    m_seats.insert(m_humanSeat, human);
}

void Table::stackDeck(const QVector<Card> &cards) {
    m_stacked = cards + m_stacked;
}

Card Table::draw() {
    return m_stacked.isEmpty() ? m_shoe.draw() : m_stacked.takeFirst();
}

QVector<TableEvent> Table::placeBets(int humanBet) {
    QVector<TableEvent> events;
    if (m_phase != Phase::Betting || !validBet(humanBet, human().bankroll))
        return events;
    for (int i = 0; i < m_seats.size(); ++i) {
        Seat &seat = m_seats[i];
        seat.hands.clear();
        const int bet = seat.isHuman ? humanBet : seat.bot.chooseBet(seat.bankroll);
        if (bet <= 0)
            continue;   // a broke bot sits this one out until replaced
        Hand hand;
        hand.bet = bet;
        seat.hands.append(hand);
        seat.bankroll -= bet;
        events.append({TableEvent::BetPlaced, QStringLiteral("%1 bets %2").arg(seat.name()).arg(bet), i, 0});
    }
    m_dealer = Hand();
    m_holeHidden = true;
    m_resolved = false;
    m_phase = Phase::Dealing;
    return events;
}

QVector<TableEvent> Table::deal() {
    QVector<TableEvent> events;
    if (m_phase != Phase::Dealing)
        return events;
    if (m_shoe.needsShuffle()) {
        m_shoe.shuffle();
        events.append({TableEvent::Shuffled, QStringLiteral("The dealer shuffles a fresh shoe")});
    }
    for (int round = 0; round < 2; ++round) {
        for (Seat &seat : m_seats)
            if (!seat.hands.isEmpty())
                seat.hands[0].cards.append(draw());
        m_dealer.cards.append(draw());
    }
    const Card up = dealerUpCard();
    events.append({TableEvent::Dealt, QStringLiteral("Dealer shows %1%2").arg(up.rankText(), up.suitGlyph())});
    if (dealerPeeks(up) && m_dealer.isBlackjack()) {
        m_holeHidden = false;
        m_phase = Phase::Payout;
        events.append({TableEvent::DealerBlackjack, QStringLiteral("Dealer has blackjack")});
        return events;
    }
    m_phase = Phase::PlayerTurns;
    m_seat = 0;
    m_hand = -1;
    moveCursor();
    if (waitingForHuman())
        events.append({TableEvent::HumanTurn, QStringLiteral("Your turn"), m_seat, m_hand});
    return events;
}

// Advances to the next hand that still needs a decision, or to the dealer.
void Table::moveCursor() {
    ++m_hand;
    while (m_seat < m_seats.size()) {
        const QVector<Hand> &hands = m_seats[m_seat].hands;
        while (m_hand < hands.size() && hands[m_hand].isFinished())
            ++m_hand;
        if (m_hand < hands.size())
            return;
        ++m_seat;
        m_hand = 0;
    }
    m_phase = Phase::DealerTurn;
}

bool Table::canAct(int seatIndex, Action action) const {
    if (m_phase != Phase::PlayerTurns || seatIndex != m_seat)
        return false;
    const Seat &seat = m_seats[seatIndex];
    const Hand &hand = seat.hands[m_hand];
    if (hand.isFinished())
        return false;
    switch (action) {
    case Action::Hit:
    case Action::Stand: return true;
    case Action::Double: return hand.canDouble() && seat.bankroll >= hand.bet;
    case Action::Split: return hand.canSplit() && seat.bankroll >= hand.bet;
    }
    return false;
}

QVector<TableEvent> Table::act(Action action) {
    if (!waitingForHuman() || !canAct(m_seat, action))
        return {};
    return apply(m_seat, action);
}

QVector<TableEvent> Table::advance() {
    switch (m_phase) {
    case Phase::PlayerTurns: {
        if (waitingForHuman())
            return {};
        Seat &seat = m_seats[m_seat];
        const Action action = seat.bot.decide(seat.hands[m_hand], dealerUpCard(),
                                              canAct(m_seat, Action::Double), canAct(m_seat, Action::Split));
        return apply(m_seat, action);
    }
    case Phase::DealerTurn: return dealerStep();
    case Phase::Payout: return resolve();
    case Phase::Betting:
    case Phase::Dealing: return {};
    }
    return {};
}

bool Table::anyLiveHand() const {
    for (const Seat &seat : m_seats)
        for (const Hand &hand : seat.hands)
            if (!hand.isBust() && !hand.isBlackjack())
                return true;
    return false;
}
