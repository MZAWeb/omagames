// Table: the parts of a round that touch cards and money — player actions,
// the dealer's draw and the payout.
#include "table.h"

using namespace BlackjackRules;

namespace {
QString cardText(const Card &c) { return c.rankText() + c.suitGlyph(); }
QString handText(const Hand &h) {
    return h.isBust() ? QStringLiteral("bust") : QString::number(h.total());
}
// "Zed hits" but "You hit": the human is addressed in the second person.
QString subject(const Seat &seat, const char *thirdPerson, const char *secondPerson) {
    return seat.name() + QLatin1Char(' ') + QString::fromUtf8(seat.isHuman ? secondPerson : thirdPerson);
}
}

QVector<TableEvent> Table::apply(int seatIndex, Action action) {
    Seat &seat = m_seats[seatIndex];
    Hand &hand = seat.hands[m_hand];
    QString text;
    switch (action) {
    case Action::Hit: {
        const Card c = draw();
        hand.cards.append(c);
        text = QStringLiteral("%1: %2 (%3)").arg(subject(seat, "hits", "hit"), cardText(c), handText(hand));
        break;
    }
    case Action::Stand:
        hand.stood = true;
        text = QStringLiteral("%1 on %2").arg(subject(seat, "stands", "stand")).arg(hand.total());
        break;
    case Action::Double: {
        seat.bankroll -= hand.bet;
        hand.bet *= 2;
        hand.doubled = true;
        const Card c = draw();
        hand.cards.append(c);
        text = QStringLiteral("%1: %2 (%3)").arg(subject(seat, "doubles", "double"), cardText(c), handText(hand));
        break;
    }
    case Action::Split: {
        seat.bankroll -= hand.bet;
        Hand second;
        second.bet = hand.bet;
        second.fromSplit = true;
        second.cards.append(hand.cards.takeLast());
        hand.fromSplit = true;
        text = QStringLiteral("%1 %2s").arg(subject(seat, "splits", "split"), hand.cards.first().rankText());
        hand.cards.append(draw());
        second.cards.append(draw());
        seat.hands.insert(m_hand + 1, second);
        break;
    }
    }
    QVector<TableEvent> events;
    events.append({TableEvent::PlayerAction, text, seatIndex, m_hand});
    if (seat.hands[m_hand].isFinished()) {
        // Re-read: the split above may have reallocated the hands vector.
        --m_hand;
        moveCursor();
        if (waitingForHuman())
            events.append({TableEvent::HumanTurn, QStringLiteral("Your turn"), m_seat, m_hand});
    }
    return events;
}

QVector<TableEvent> Table::dealerStep() {
    if (m_holeHidden) {
        m_holeHidden = false;
        return {{TableEvent::DealerReveal,
                 QStringLiteral("Dealer reveals %1 (%2)").arg(cardText(m_dealer.cards[1]), handText(m_dealer))}};
    }
    // Nobody left to beat: no point drawing cards that could only bust the dealer.
    if (anyLiveHand() && dealerShouldHit(m_dealer)) {
        const Card c = draw();
        m_dealer.cards.append(c);
        return {{TableEvent::DealerCard, QStringLiteral("Dealer draws %1 (%2)").arg(cardText(c), handText(m_dealer))}};
    }
    m_phase = Phase::Payout;
    const QString text = m_dealer.isBust() ? QStringLiteral("Dealer busts")
                                           : QStringLiteral("Dealer stands on %1").arg(m_dealer.total());
    return {{TableEvent::DealerStand, text}};
}

QVector<TableEvent> Table::resolve() {
    QVector<TableEvent> events;
    if (m_resolved)
        return events;
    for (int s = 0; s < m_seats.size(); ++s) {
        Seat &seat = m_seats[s];
        for (int h = 0; h < seat.hands.size(); ++h) {
            Hand &hand = seat.hands[h];
            hand.returned = payout(hand, m_dealer);
            hand.resolved = true;
            seat.bankroll += hand.returned;
            const int net = hand.returned - hand.bet;
            QString text;
            switch (outcome(hand, m_dealer)) {
            case Outcome::Blackjack: text = QStringLiteral("%1 blackjack! +%2").arg(seat.name()); break;
            case Outcome::Win: text = QStringLiteral("%1 +%2").arg(subject(seat, "wins", "win")); break;
            case Outcome::Push: text = QStringLiteral("%1 %2").arg(subject(seat, "pushes", "push")); break;
            case Outcome::Lose: text = QStringLiteral("%1 %2").arg(subject(seat, "loses", "lose")); break;
            }
            events.append({TableEvent::HandResolved, text.arg(qAbs(net)), s, h});
        }
    }
    m_resolved = true;
    return events;
}

QVector<TableEvent> Table::nextRound(QRandomGenerator &rng) {
    QVector<TableEvent> events;
    if (!roundOver())
        return events;
    for (int i = 0; i < m_seats.size(); ++i) {
        Seat &seat = m_seats[i];
        seat.hands.clear();
        if (seat.isHuman || !seat.broke())
            continue;
        events.append({TableEvent::BotLeft, QStringLiteral("%1 is broke and leaves the table").arg(seat.name()), i});
        const BotPersonality fresh = BotPersonality::roll(takenNames(), rng);
        seat.bot = BotPlayer(fresh, rng.generate());
        seat.bankroll = kStartingBankroll;
        events.append({TableEvent::BotJoined, QStringLiteral("%1 sits down (%2)").arg(fresh.name, fresh.label()), i});
    }
    m_dealer = Hand();
    m_holeHidden = true;
    m_phase = Phase::Betting;
    return events;
}
