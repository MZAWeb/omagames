#include "insurancetests.h"

#include <QtTest>

#include "basicstrategy.h"
#include "blackjackgame.h"
#include "blackjackrules.h"
#include "botplayer.h"
#include "hand.h"
#include "table.h"
#include "testhelpers.h"

void InsuranceTests::cleanup() {
    QSettings().clear();
}

void InsuranceTests::resplitsToFourHandsAndStopsThere() {
    Table t(1);
    t.stackDeck(cards({8, 9, 8, 7, 8, 8, 8, 10, 10, 10, 5, 10}));
    t.placeBets(50);
    dealOut(t);
    for (int splits = 0; splits < 3; ++splits) {
        QVERIFY(t.canAct(t.humanSeat(), Table::Action::Split));
        t.act(Table::Action::Split);
        QCOMPARE(t.currentHand(), 0);          // the fresh pair is played first
    }
    const Seat &me = t.human();
    QCOMPARE(me.hands.size(), BlackjackRules::kMaxHandsPerSeat);
    QCOMPARE(me.bankroll, 800);                // four stakes of 50 are down
    t.act(Table::Action::Stand);
    t.act(Table::Action::Stand);
    t.act(Table::Action::Stand);
    QCOMPARE(t.currentHand(), 3);
    QVERIFY(me.hands[3].canSplit());           // a pair, but the seat is full
    QVERIFY(!t.canAct(t.humanSeat(), Table::Action::Split));
    t.act(Table::Action::Hit);
    autoplay(t);
    QVERIFY(t.dealer().isBust());
    for (const Hand &h : me.hands)
        QCOMPARE(h.returned, 100);
    QCOMPARE(me.bankroll, 1200);
}

void InsuranceTests::splitAcesAreNeverResplit() {
    Table t(1);
    t.stackDeck(cards({1, 9, 1, 7, 1, 1, 10}));
    t.placeBets(50);
    dealOut(t);
    t.act(Table::Action::Split);
    const Seat &me = t.human();
    QCOMPARE(me.hands.size(), 2);
    QVERIFY(!me.hands[0].canSplit());          // A,A after a split still stands pat
    QVERIFY(!me.hands[1].canSplit());
    QCOMPARE(t.phase(), Table::Phase::DealerTurn);
    autoplay(t);
    QCOMPARE(me.bankroll, 1100);               // both 12s ride out a dealer bust
}

void InsuranceTests::basicStrategyResplitsPairs() {
    Hand again = hand({8, 8});
    again.fromSplit = true;
    QCOMPARE(BasicStrategy::decide(again, c(10), false, true), BlackjackRules::Action::Split);
    QCOMPARE(BasicStrategy::decide(again, c(10), false, false), BlackjackRules::Action::Hit);
}

void InsuranceTests::botsResplitAtTheTable() {
    Table t(1);
    t.addBot(perfect(QStringLiteral("Zed")));
    t.stackDeck(cards({10, 8, 9, 9, 8, 7, 8, 5, 10, 10, 10, 10}));
    t.placeBets(50);
    dealOut(t);
    QVERIFY(t.waitingForHuman());
    t.act(Table::Action::Stand);
    autoplay(t);
    QVERIFY(t.roundOver());
    QCOMPARE(t.seats()[1].hands.size(), 3);    // 8,8 split, then split again
}

void InsuranceTests::insuranceStakeAndReturn() {
    Hand natural = hand({1, 10});
    Hand plain = hand({10, 7});
    QCOMPARE(BlackjackRules::insuranceStake(100), 50);
    QCOMPARE(BlackjackRules::insuranceReturn(50, natural), 150);   // stake plus 2 to 1
    QCOMPARE(BlackjackRules::insuranceReturn(50, plain), 0);
}

void InsuranceTests::insuranceIsOfferedOnlyAgainstAnAce() {
    Table ten(1);
    ten.stackDeck(cards({10, 10, 9, 7}));
    ten.placeBets(50);
    dealOut(ten);
    QVERIFY(!ten.waitingForInsurance());
    QCOMPARE(ten.phase(), Table::Phase::PlayerTurns);
    QVERIFY(ten.takeInsurance().isEmpty());

    Table ace(1);
    ace.stackDeck(cards({10, 1, 9, 7}));
    ace.placeBets(50);
    dealOut(ace);
    QCOMPARE(ace.phase(), Table::Phase::Insurance);
    QVERIFY(ace.waitingForInsurance());
    QVERIFY(ace.canInsure(ace.humanSeat()));
    QCOMPARE(ace.insuranceCost(ace.humanSeat()), 25);
    QVERIFY(ace.act(Table::Action::Hit).isEmpty());   // no cards until it is answered
}

void InsuranceTests::insurancePaysTwoToOneOnADealerNatural() {
    Table t(1);
    t.stackDeck(cards({10, 1, 9, 13}));
    t.placeBets(100);
    dealOut(t);
    QCOMPARE(t.human().bankroll, 900);
    const auto events = t.takeInsurance();
    QCOMPARE(t.human().insuranceBet, 50);
    QCOMPARE(t.human().insuranceReturned, 150);
    QCOMPARE(t.human().bankroll, 1000);
    QCOMPARE(events.first().text, QStringLiteral("You take insurance"));
    QCOMPARE(events.last().type, TableEvent::InsuranceResolved);
    QCOMPARE(events.last().text, QStringLiteral("Insurance pays Ø 100"));
    autoplay(t);
    QVERIFY(t.roundOver());
    QCOMPARE(t.human().hands[0].returned, 0);        // the hand itself is gone
    QCOMPARE(t.human().bankroll, 1000);              // insurance made it a wash
}

void InsuranceTests::insuranceIsLostWithoutADealerNatural() {
    Table t(1);
    t.stackDeck(cards({10, 1, 9, 7}));
    t.placeBets(100);
    dealOut(t);
    const auto events = t.takeInsurance();
    QCOMPARE(t.human().insuranceBet, 50);
    QCOMPARE(t.human().insuranceReturned, 0);
    QCOMPARE(t.human().bankroll, 850);
    QCOMPARE(events.first().type, TableEvent::Insurance);
    QCOMPARE(events[1].type, TableEvent::InsuranceResolved);
    QCOMPARE(events[1].text, QStringLiteral("Insurance loses Ø 50"));
    QCOMPARE(t.phase(), Table::Phase::PlayerTurns);
    t.act(Table::Action::Stand);
    autoplay(t);
    QCOMPARE(t.dealer().total(), 18);
    QCOMPARE(t.human().bankroll, 1050);              // 19 beats 18, minus the side bet
}

void InsuranceTests::insuranceIsSkippedWhenNobodyCanCoverIt() {
    Table t(1);
    t.setHumanBankroll(10);
    t.addBot(perfect(QStringLiteral("Zed")), 5);      // too broke to have a bet
    t.stackDeck(cards({10, 1, 9, 7}));
    t.placeBets(10);
    dealOut(t);
    QVERIFY(t.seats()[1].hands.isEmpty());
    QVERIFY(!t.waitingForInsurance());
    QCOMPARE(t.phase(), Table::Phase::PlayerTurns);
    QVERIFY(t.takeInsurance().isEmpty());
}

void InsuranceTests::botInsuranceFollowsSkillDeterministically() {
    BotPlayer rookie({QStringLiteral("Nina"), 0.05, 0.9, 7}), twin({QStringLiteral("Nina"), 0.05, 0.9, 7});
    BotPlayer pro(perfect(QStringLiteral("Ace")));
    int taken = 0;
    for (int i = 0; i < 50; ++i) {
        const bool bite = rookie.takesInsurance();
        QCOMPARE(bite, twin.takesInsurance());
        taken += bite ? 1 : 0;
        QVERIFY(!pro.takesInsurance());               // the book never insures
    }
    QVERIFY(taken > 0 && taken < 50);
}

void InsuranceTests::botAnswersTheInsuranceOfferAtTheTable() {
    const BotPersonality nina{QStringLiteral("Nina"), 0.0, 1.0, 7};
    BotPlayer oracle(nina);
    const bool expected = oracle.takesInsurance();
    Table t(1);
    t.addBot(nina);
    t.stackDeck(cards({10, 5, 1, 9, 6, 7}));
    t.placeBets(100);
    dealOut(t);
    QVERIFY(t.waitingForInsurance());                 // the human is asked first
    t.declineInsurance();
    QCOMPARE(t.human().insuranceBet, 0);
    QCOMPARE(t.phase(), Table::Phase::Insurance);
    const auto events = t.advance();
    const int stake = BlackjackRules::insuranceStake(t.seats()[1].hands[0].bet);
    QCOMPARE(t.seats()[1].insuranceBet, expected ? stake : 0);
    QCOMPARE(events.first().text, expected ? QStringLiteral("Nina takes insurance") : QString());
    QCOMPARE(t.phase(), Table::Phase::PlayerTurns);
}

// The dock picks its tray from exactly these five flags, so pinning them
// here pins what the player is offered: the insurance pair and nothing else.
void InsuranceTests::bridgeOffersInsuranceAgainstAnAce() {
    QSettings().clear();
    BlackjackGame g;
    g.setStepInterval(0);
    g.setBotCount(0);
    g.setCoachEnabled(true);
    g.setBet(100);
    g.stackDeck(cards({10, 1, 9, 13}));
    g.dealRound();
    QCOMPARE(g.phase(), QStringLiteral("insurance"));
    QVERIFY(g.canInsure());          // the Insurance / No insurance pair shows
    QVERIFY(!g.waitingForHuman());   // and the Hit / Stand / Double / Split row does not
    QVERIFY(!g.roundOver());         // nor Next round
    QVERIFY(!g.canDeal());           // nor the bet controls
    QCOMPARE(g.insuranceCost(), 50);
    QCOMPARE(g.coachAction(), QStringLiteral("No insurance"));
    QCOMPARE(g.coachSituation(), QStringLiteral("Dealer shows an ace"));
    g.insurance();
    QVERIFY(!g.canInsure());
    QVERIFY(g.roundOver());
    QVERIFY(g.log().contains(QStringLiteral("You take insurance")));
    QVERIFY(g.log().contains(QStringLiteral("Insurance pays Ø 100")));
    QCOMPARE(g.bankroll(), 1000);      // the side bet exactly covered the lost hand
    QCOMPARE(g.netResult(), 0);
}

// The seats model carries the side bet so the table can draw it as a chip:
// down and unsettled from the moment it is placed, then won or lost at the
// peek, and gone again the next round.
void InsuranceTests::seatsModelCarriesTheInsuranceSideBet() {
    QSettings().clear();
    BlackjackGame g;
    g.setStepInterval(0);
    g.setBotCount(0);
    g.setBet(100);
    g.stackDeck(cards({10, 1, 9, 13}));   // dealer shows an ace, holds a natural
    g.dealRound();
    auto humanSeat = [&g] { return g.seats()[g.humanSeat()].toMap(); };
    QCOMPARE(humanSeat()[QStringLiteral("insuranceBet")].toInt(), 0);
    QCOMPARE(humanSeat()[QStringLiteral("insuranceSettled")].toBool(), false);

    g.insurance();
    QCOMPARE(humanSeat()[QStringLiteral("insuranceBet")].toInt(), 50);
    QCOMPARE(humanSeat()[QStringLiteral("insuranceSettled")].toBool(), true);
    QCOMPARE(humanSeat()[QStringLiteral("insuranceNet")].toInt(), 100);   // pays 2 to 1

    g.nextRound();
    QCOMPARE(humanSeat()[QStringLiteral("insuranceBet")].toInt(), 0);
    QCOMPARE(humanSeat()[QStringLiteral("insuranceSettled")].toBool(), false);
    QCOMPARE(humanSeat()[QStringLiteral("insuranceNet")].toInt(), 0);

    // and a side bet the dealer's hole card kills reads as a loss
    BlackjackGame lost;
    lost.setStepInterval(0);
    lost.setBotCount(0);
    lost.setBet(100);
    lost.stackDeck(cards({10, 1, 9, 7}));
    lost.dealRound();
    lost.insurance();
    const QVariantMap seat = lost.seats()[lost.humanSeat()].toMap();
    QCOMPARE(seat[QStringLiteral("insuranceBet")].toInt(), 50);
    QCOMPARE(seat[QStringLiteral("insuranceSettled")].toBool(), true);
    QCOMPARE(seat[QStringLiteral("insuranceNet")].toInt(), -50);
}

void InsuranceTests::bridgeDeclinedInsuranceCostsNothing() {
    QSettings().clear();
    BlackjackGame g;
    g.setStepInterval(0);
    g.setBotCount(0);
    g.setBet(100);
    g.stackDeck(cards({10, 1, 9, 13}));
    g.dealRound();
    QVERIFY(g.canInsure());
    g.declineInsurance();
    QVERIFY(g.roundOver());
    QCOMPARE(g.bankroll(), 900);
    QCOMPARE(g.netResult(), -100);
    QVERIFY(g.log().contains(QStringLiteral("Insurance?")));   // the offer still shows
    for (const QString &line : g.log()) {
        QVERIFY(!line.contains(QStringLiteral("take insurance")));
        QVERIFY(!line.startsWith(QStringLiteral("Insurance ")));
    }
}
