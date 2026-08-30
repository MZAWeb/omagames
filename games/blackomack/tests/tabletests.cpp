#include "tabletests.h"

#include <QtTest>

#include "blackjackrules.h"
#include "basicstrategy.h"
#include "hand.h"
#include "seatlayout.h"
#include "table.h"
#include "testhelpers.h"

void TableTests::openingDealGoesOutOneCardAtATime() {
    Table t(1);
    t.addBot(perfect(QStringLiteral("Zed")));
    t.addBot(perfect(QStringLiteral("Mona")));       // the human's tray sits between them
    t.stackDeck(cards({2, 3, 4, 5, 6, 7, 8, 9}));
    t.placeBets(10);
    QCOMPARE(t.phase(), Table::Phase::Dealing);
    QVERIFY(t.dealer().cards.isEmpty());

    QVector<int> order;
    int cardsOut = 0;
    while (t.phase() == Table::Phase::Dealing) {
        const auto events = t.advance();
        QVERIFY(!events.isEmpty());
        QCOMPARE(events.first().type, TableEvent::Dealt);
        int dealt = 0;
        for (const TableEvent &e : events)
            if (e.type == TableEvent::Dealt)
                ++dealt;
        QCOMPARE(dealt, 1);                          // exactly one card per step
        order.append(events.first().seat);
        ++cardsOut;
        int onTheFelt = t.dealer().cards.size();
        for (const Seat &s : t.seats())
            for (const Hand &h : s.hands)
                onTheFelt += h.cards.size();
        QCOMPARE(onTheFelt, cardsOut);
        QVERIFY(t.act(Table::Action::Hit).isEmpty()); // nobody plays mid-deal
    }
    // One card to every seat in turn, then the dealer, twice around.
    QCOMPARE(order, QVector<int>({0, 1, 2, Table::kDealerSeat, 0, 1, 2, Table::kDealerSeat}));
    QCOMPARE(t.seats()[0].hands[0].cards, cards({2, 6}));
    QCOMPARE(t.human().hands[0].cards, cards({3, 7}));
    QCOMPARE(t.seats()[2].hands[0].cards, cards({4, 8}));
    QCOMPARE(t.dealer().cards, cards({5, 9}));
    QVERIFY(t.holeHidden());                         // the hole card waits for the reveal
    QCOMPARE(t.phase(), Table::Phase::PlayerTurns);
    QCOMPARE(t.currentSeat(), 0);                    // the seat at the dealer's left decides first
}

void TableTests::naturalPaysThreeToTwo() {
    Table t(1);
    t.stackDeck(cards({1, 7, 13, 10}));
    QCOMPARE(t.placeBets(50).size(), 1);
    QCOMPARE(t.human().bankroll, 950);
    dealOut(t);
    QCOMPARE(t.phase(), Table::Phase::DealerTurn);   // nothing for the human to decide
    QVERIFY(t.holeHidden());
    autoplay(t);
    QVERIFY(t.roundOver());
    QVERIFY(!t.holeHidden());
    QCOMPARE(t.dealer().cards.size(), 2);            // no draw with nobody to beat
    QCOMPARE(t.human().hands[0].returned, 125);
    QCOMPARE(t.human().bankroll, 1075);
}

void TableTests::dealerPeekEndsRound() {
    Table t(1);
    t.stackDeck(cards({10, 1, 9, 13}));
    t.placeBets(50);
    dealOut(t);
    QVERIFY(t.waitingForInsurance());          // an ace up asks before it peeks
    const auto events = t.declineInsurance();
    QCOMPARE(t.phase(), Table::Phase::Payout);
    QVERIFY(!t.holeHidden());
    QCOMPARE(events.last().type, TableEvent::DealerBlackjack);
    QVERIFY(t.act(Table::Action::Hit).isEmpty());
    autoplay(t);
    QCOMPARE(t.human().bankroll, 950);
    QCOMPARE(t.human().hands[0].returned, 0);
}

void TableTests::doubleTakesExactlyOneCard() {
    Table t(1);
    t.stackDeck(cards({5, 6, 6, 10, 10, 9}));
    t.placeBets(100);
    dealOut(t);
    QVERIFY(t.waitingForHuman());
    QVERIFY(t.canAct(t.humanSeat(), Table::Action::Double));
    QVERIFY(!t.canAct(t.humanSeat(), Table::Action::Split));
    t.act(Table::Action::Double);
    QCOMPARE(t.human().hands[0].cards.size(), 3);
    QCOMPARE(t.human().hands[0].bet, 200);
    QCOMPARE(t.human().bankroll, 800);
    QVERIFY(!t.waitingForHuman());
    autoplay(t);
    QVERIFY(t.dealer().isBust());
    QCOMPARE(t.human().bankroll, 1200);
}

void TableTests::splitAcesGetOneCardEach() {
    Table t(1);
    t.stackDeck(cards({1, 9, 1, 7, 10, 5, 2}));
    t.placeBets(50);
    dealOut(t);
    QVERIFY(t.canAct(t.humanSeat(), Table::Action::Split));
    t.act(Table::Action::Split);
    const Seat &me = t.human();
    QCOMPARE(me.hands.size(), 2);
    QCOMPARE(me.hands[0].cards.size(), 2);
    QCOMPARE(me.hands[1].cards.size(), 2);
    QCOMPARE(me.bankroll, 900);
    QVERIFY(!t.waitingForHuman());               // both hands are done: no hitting split aces
    QCOMPARE(t.phase(), Table::Phase::DealerTurn);
    autoplay(t);
    QCOMPARE(t.dealer().total(), 18);
    QCOMPARE(me.hands[0].returned, 100);         // A+10 after a split is 21, not a natural
    QCOMPARE(me.hands[1].returned, 0);
    QCOMPARE(me.bankroll, 1000);
}

void TableTests::splitHandsPlayInOrderAndMayDouble() {
    Table t(1);
    t.stackDeck(cards({8, 6, 8, 9, 3, 10, 10, 10}));
    t.placeBets(50);
    dealOut(t);
    t.act(Table::Action::Split);
    QCOMPARE(t.currentHand(), 0);
    QVERIFY(t.canAct(t.humanSeat(), Table::Action::Double));   // double after split
    QVERIFY(!t.canAct(t.humanSeat(), Table::Action::Split));   // 8,3 is no pair
    t.act(Table::Action::Double);
    QCOMPARE(t.currentHand(), 1);
    QVERIFY(t.waitingForHuman());
    t.act(Table::Action::Stand);
    autoplay(t);
    QVERIFY(t.dealer().isBust());
    QCOMPARE(t.human().bankroll, 1000 + 100 + 50);
}

void TableTests::dealerStandsOnSoft17AtTheTable() {
    Table t(1);
    t.stackDeck(cards({10, 1, 8, 6}));
    t.placeBets(10);
    dealOut(t);
    t.declineInsurance();
    t.act(Table::Action::Stand);
    autoplay(t);
    QCOMPARE(t.dealer().cards.size(), 2);
    QCOMPARE(t.dealer().total(), 17);
    QCOMPARE(t.human().bankroll, 1010);
}

void TableTests::doubleAndSplitNeedFunds() {
    Table t(1);
    t.setHumanBankroll(60);
    t.stackDeck(cards({8, 6, 8, 9}));
    t.placeBets(50);
    dealOut(t);
    QVERIFY(t.waitingForHuman());
    QVERIFY(!t.canAct(t.humanSeat(), Table::Action::Double));
    QVERIFY(!t.canAct(t.humanSeat(), Table::Action::Split));
    QVERIFY(t.act(Table::Action::Split).isEmpty());
    QVERIFY(t.canAct(t.humanSeat(), Table::Action::Hit));
}

// The human sits where the sweep round the oval reaches the bottom-centre
// tray, so the seating chart — not the array — decides the index, and the
// mates keep the order they sat down in on either side of it.
void TableTests::seatingFollowsTheArcAroundTheTable() {
    Table t(1);
    QCOMPARE(t.humanSeat(), 0);
    QVERIFY(t.addBot(perfect(QStringLiteral("Zed"))));
    QCOMPARE(t.humanSeat(), 0);             // the lone mate takes the far side
    QVERIFY(t.addBot(perfect(QStringLiteral("Mona"))));
    QCOMPARE(t.humanSeat(), 1);
    t.addBot(perfect(QStringLiteral("Bucky")));
    QCOMPARE(t.humanSeat(), 1);
    t.addBot(perfect(QStringLiteral("Ivy")));
    QCOMPARE(t.humanSeat(), 2);             // two mates ahead of you, two behind
    t.addBot(perfect(QStringLiteral("Rex")));
    QVERIFY(t.addBot(perfect(QStringLiteral("Lola"))));
    QCOMPARE(t.botCount(), 6);
    QVERIFY(!t.addBot(perfect(QStringLiteral("Nina"))));
    QCOMPARE(t.humanSeat(), 3);
    QCOMPARE(t.humanSeat(), SeatLayout::matesBeforeHuman(t.botCount()));
    QCOMPARE(t.takenNames(), QStringList({QStringLiteral("Zed"), QStringLiteral("Mona"), QStringLiteral("Bucky"),
                                          QStringLiteral("Ivy"), QStringLiteral("Rex"), QStringLiteral("Lola")}));
    QVERIFY(t.removeLastBot());
    QCOMPARE(t.botCount(), 5);
    QCOMPARE(t.humanSeat(), SeatLayout::matesBeforeHuman(5));
    QVERIFY(t.human().isHuman);
    QCOMPARE(t.takenNames(), QStringList({QStringLiteral("Zed"), QStringLiteral("Mona"), QStringLiteral("Bucky"), QStringLiteral("Ivy"), QStringLiteral("Rex")}));
}

void TableTests::fullRoundsWithBotsBalanceTheBooks() {
    Table t(2024);
    QRandomGenerator rng(1);
    t.addBot({QStringLiteral("Zed"), 0.9, 0.9, 11});
    t.addBot({QStringLiteral("Moe"), 0.1, 0.2, 12});
    t.addBot({QStringLiteral("Ivy"), 0.5, 0.5, 13});
    QVERIFY(t.placeBets(15).isEmpty());          // not a multiple of 10
    QVERIFY(t.placeBets(2000).isEmpty());        // more than the bankroll
    for (int round = 0; round < 40; ++round) {
        QVector<int> before;
        for (const Seat &s : t.seats())
            before.append(s.bankroll);
        QVERIFY(!t.placeBets(20).isEmpty());
        QVERIFY(!t.addBot(perfect(QStringLiteral("Rex"))));   // not mid-round
        dealOut(t);
        while (!t.roundOver()) {
            autoplay(t);
            if (t.waitingForHuman())
                t.act(BasicStrategy::decide(t.human().hands[t.currentHand()], t.dealerUpCard(),
                                            t.canAct(t.humanSeat(), Table::Action::Double),
                                            t.canAct(t.humanSeat(), Table::Action::Split)));
        }
        for (int i = 0; i < t.seats().size(); ++i) {
            int staked = t.seats()[i].insuranceBet;
            int returned = t.seats()[i].insuranceReturned;
            for (const Hand &h : t.seats()[i].hands) {
                QVERIFY(h.resolved);
                staked += h.bet;
                returned += h.returned;
            }
            QCOMPARE(t.seats()[i].bankroll, before[i] - staked + returned);
        }
        t.nextRound(rng);
        QCOMPARE(t.phase(), Table::Phase::Betting);
    }
}

void TableTests::brokeBotIsReplacedNextRound() {
    Table t(3);
    QRandomGenerator rng(9);
    t.addBot(perfect(QStringLiteral("Zed")), 5);
    for (int i = 1; i < BlackjackRules::kMaxBots; ++i)
        QVERIFY(t.addBot(perfect(QStringLiteral("Mate %1").arg(i))));
    QCOMPARE(t.botCount(), BlackjackRules::kMaxBots);
    t.placeBets(10);
    int brokeSeat = -1;
    for (int i = 0; i < t.seats().size(); ++i)
        if (t.seats()[i].name() == QStringLiteral("Zed"))
            brokeSeat = i;
    QVERIFY(brokeSeat >= 0);
    QVERIFY(t.seats()[brokeSeat].hands.isEmpty());   // can't afford the minimum: sits out
    dealOut(t);
    while (!t.roundOver()) {
        autoplay(t);
        if (t.waitingForHuman())
            t.act(Table::Action::Stand);
    }
    const auto events = t.nextRound(rng);
    QCOMPARE(events.size(), 2);
    QCOMPARE(events[0].type, TableEvent::BotLeft);
    QCOMPARE(events[1].type, TableEvent::BotJoined);
    QCOMPARE(t.botCount(), BlackjackRules::kMaxBots);
    QVERIFY(t.seats()[brokeSeat].bot.personality().name != QStringLiteral("Zed"));
    QCOMPARE(t.seats()[brokeSeat].bankroll, 1000);
}

void TableTests::nextRoundOnlyAfterPayout() {
    Table t(1);
    QRandomGenerator rng(1);
    QVERIFY(t.nextRound(rng).isEmpty());
    QVERIFY(t.advance().isEmpty());              // nothing automatic while betting
    t.placeBets(10);
    QVERIFY(t.nextRound(rng).isEmpty());
}
