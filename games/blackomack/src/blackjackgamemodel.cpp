// BlackjackGame: flattening table state into the variant maps QML renders.
#include "blackjackgame.h"

using namespace BlackjackRules;

namespace {

QVariantMap cardMap(const Card &c) {
    return {
        {QStringLiteral("rank"), c.rankText()},
        {QStringLiteral("suit"), c.suitGlyph()},
        {QStringLiteral("red"), c.isRed()},
    };
}

QString resultText(const Hand &hand, const Hand &dealer) {
    if (hand.isBust())
        return QStringLiteral("BUST");
    if (!hand.resolved)
        return hand.isBlackjack() ? QStringLiteral("BJ") : QString();
    switch (outcome(hand, dealer)) {
    case Outcome::Blackjack: return QStringLiteral("BJ");
    case Outcome::Win: return QStringLiteral("WIN");
    case Outcome::Push: return QStringLiteral("PUSH");
    case Outcome::Lose: return QStringLiteral("LOSE");
    }
    return QString();
}

QVariantMap handMap(const Hand &hand, const Hand &dealer, bool active) {
    QVariantList cards;
    for (const Card &c : hand.cards)
        cards.append(cardMap(c));
    return {
        {QStringLiteral("cards"), cards},
        {QStringLiteral("total"), hand.total()},
        {QStringLiteral("soft"), hand.isSoft()},
        {QStringLiteral("bet"), hand.bet},
        {QStringLiteral("active"), active},
        {QStringLiteral("finished"), hand.isFinished()},
        {QStringLiteral("result"), resultText(hand, dealer)},
        {QStringLiteral("resolved"), hand.resolved},
        {QStringLiteral("net"), hand.resolved ? hand.returned - hand.bet : 0},
    };
}

}

QVariantList BlackjackGame::seats() const {
    QVariantList list;
    const QVector<Seat> &seats = m_table.seats();
    for (int s = 0; s < seats.size(); ++s) {
        const Seat &seat = seats[s];
        QVariantList hands;
        for (int h = 0; h < seat.hands.size(); ++h)
            hands.append(handMap(seat.hands[h], m_table.dealer(),
                                 s == m_table.currentSeat() && h == m_table.currentHand()));
        list.append(QVariantMap{
            {QStringLiteral("name"), seat.name()},
            {QStringLiteral("human"), seat.isHuman},
            {QStringLiteral("personality"), seat.isHuman ? QString() : seat.bot.personality().label()},
            {QStringLiteral("bankroll"), seat.bankroll},
            {QStringLiteral("net"), seat.isHuman ? m_netResult : seat.bankroll - kStartingBankroll},
            {QStringLiteral("hands"), hands},
            {QStringLiteral("active"), s == m_table.currentSeat()},
        });
    }
    return list;
}

QVariantMap BlackjackGame::dealerHand() const {
    const Hand &dealer = m_table.dealer();
    QVariantList cards;
    for (int i = 0; i < dealer.cards.size(); ++i) {
        QVariantMap c = cardMap(dealer.cards[i]);
        c.insert(QStringLiteral("hidden"), i == 1 && m_table.holeHidden());
        cards.append(c);
    }
    Hand visible = dealer;
    if (m_table.holeHidden() && visible.cards.size() > 1)
        visible.cards.resize(1);
    return {
        {QStringLiteral("cards"), cards},
        {QStringLiteral("total"), visible.total()},
        {QStringLiteral("bust"), dealer.isBust()},
        {QStringLiteral("blackjack"), !m_table.holeHidden() && dealer.isBlackjack()},
    };
}
