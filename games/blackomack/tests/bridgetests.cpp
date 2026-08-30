#include "bridgetests.h"

#include <QtTest>

#include "blackjackgame.h"
#include "blackjackrules.h"
#include "cards.h"
#include "hand.h"
#include "testhelpers.h"

void BridgeTests::cleanup() {
    QSettings().clear();
}

void BridgeTests::bridgeBetting() {
    BlackjackGame g;
    g.setStepInterval(0);
    QCOMPARE(g.bankroll(), 1000);
    QCOMPARE(g.minBet(), 10);
    QCOMPARE(g.maxBet(), 1000);
    g.setBet(35);
    QCOMPARE(g.bet(), 30);
    g.adjustBet(-100);
    QCOMPARE(g.bet(), 10);
    g.betMax();
    QCOMPARE(g.bet(), 1000);
    g.setBet(5000);
    QCOMPARE(g.bet(), 1000);
    QVERIFY(g.canDeal());
    QCOMPARE(g.phase(), QStringLiteral("betting"));
    QCOMPARE(g.shoeRemaining(), 312);
    QCOMPARE(g.shoePercent(), 100);
    QCOMPARE(g.rulesSummary(),
             QStringLiteral("Blackjack pays 3 to 2 · Dealer stands on all 17"));
}

void BridgeTests::bridgeSeatsBotsOnAFreshTable() {
    {
        BlackjackGame fresh;
        QCOMPARE(fresh.botCount(), 4);
        QCOMPARE(fresh.maxBots(), 6);
        fresh.setBotCount(9);
        QCOMPARE(fresh.botCount(), 6);      // the table only has six other seats
        fresh.setBotCount(-2);
        QCOMPARE(fresh.botCount(), 0);      // playing alone is a choice
    }
    BlackjackGame again;                    // ... and it survives a relaunch
    QCOMPARE(again.botCount(), 0);
}

void BridgeTests::bridgeCoachDescribesKnownHandsOnlyOnYourTurn() {
    struct Case {
        QVector<Card> deck;
        QString action;
        QString situation;
    };
    // The wording is the player's, not the strategy table's: a verb and
    // the spot it applies to, with the article the dealer's card takes.
    const QVector<Case> cases = {
        {cards({10, 10, 6, 7}), QStringLiteral("Hit"), QStringLiteral("16 against a 10")},
        {cards({10, 8, 6, 7}), QStringLiteral("Hit"), QStringLiteral("16 against an 8")},
        {cards({10, 1, 6, 7}), QStringLiteral("Hit"), QStringLiteral("16 against an ace")},
        {cards({1, 3, 7, 9}), QStringLiteral("Double"), QStringLiteral("Soft 18 against a 3")},
        {cards({8, 10, 8, 7}), QStringLiteral("Split"), QStringLiteral("Pair of 8s against a 10")},
        {cards({1, 6, 1, 9}), QStringLiteral("Split"), QStringLiteral("Pair of aces against a 6")},
    };
    for (const Case &test : cases) {
        QSettings().clear();
        BlackjackGame g;
        g.setStepInterval(0);
        g.setBotCount(0);
        g.setCoachEnabled(true);
        g.stackDeck(test.deck);
        QVERIFY(g.coachAction().isEmpty());
        QVERIFY(g.coachSituation().isEmpty());
        g.dealRound();
        if (g.canInsure())
            g.declineInsurance();
        QVERIFY(g.waitingForHuman());
        QCOMPARE(g.shoeRemaining(), 312);   // scripted cards do not consume the real shoe
        QCOMPARE(g.shoePercent(), 100);
        QCOMPARE(g.coachAction(), test.action);
        QCOMPARE(g.coachSituation(), test.situation);
        g.stand();
        QVERIFY(g.coachAction().isEmpty());
        QVERIFY(g.coachSituation().isEmpty());
    }

    QSettings().clear();
    BlackjackGame splitGame;
    splitGame.setStepInterval(0);
    splitGame.setBotCount(0);
    splitGame.setCoachEnabled(true);
    splitGame.stackDeck(cards({8, 6, 8, 10, 3, 2}));
    splitGame.dealRound();
    splitGame.split();
    QCOMPARE(splitGame.coachAction(), QStringLiteral("Double"));
    QCOMPARE(splitGame.coachSituation(), QStringLiteral("Hand 1 of 2 · 11 against a 6"));
}

void BridgeTests::bridgeCoachSaysNothingWhileItIsOff() {
    QSettings().clear();
    BlackjackGame g;
    g.setStepInterval(0);
    g.setBotCount(0);
    g.stackDeck(cards({10, 10, 6, 7}));
    g.dealRound();
    QVERIFY(g.waitingForHuman());
    QVERIFY(g.coachAction().isEmpty());     // off is off, even mid-decision
    g.setCoachEnabled(true);
    QCOMPARE(g.coachAction(), QStringLiteral("Hit"));
}

void BridgeTests::bridgeSkipPacingStopsBeforeTheHumanDecision() {
    BlackjackGame g;
    g.setBotCount(2);
    g.stackDeck(cards({10, 10, 10, 6, 7, 6, 7, 10}));
    g.dealRound();
    QCOMPARE(g.phase(), QStringLiteral("dealing"));
    QVERIFY(!g.waitingForHuman());
    g.skipPacing();
    QCOMPARE(g.phase(), QStringLiteral("playing"));
    QVERIFY(g.waitingForHuman());
    QVERIFY(g.canHit());
}

void BridgeTests::bridgeLocksTheStakeOnceDealt() {
    BlackjackGame g;
    g.setStepInterval(0);
    g.setBet(50);
    g.dealRound();
    QVERIFY(g.phase() != QStringLiteral("betting"));
    g.setBet(100);
    QCOMPARE(g.bet(), 50);
    g.adjustBet(10);
    QCOMPARE(g.bet(), 50);
    g.betMax();
    QCOMPARE(g.bet(), 50);
    while (!g.roundOver())
        g.canInsure() ? g.declineInsurance() : g.stand();
    g.setBet(80);                // still locked while the result is on the table
    QCOMPARE(g.bet(), 50);
    g.nextRound();
    g.setBet(80);
    QCOMPARE(g.bet(), 80);
}

// The payout screen reads its amounts straight off the model.
void BridgeTests::bridgeReportsWhatEachHandPaid() {
    BlackjackGame g;
    g.setStepInterval(0);
    g.setBotCount(2);
    g.setBet(20);
    playRound(g);
    int handsSeen = 0;
    for (const QVariant &seatValue : g.seats()) {
        const QVariantList hands = seatValue.toMap().value(QStringLiteral("hands")).toList();
        for (const QVariant &handValue : hands) {
            const QVariantMap hand = handValue.toMap();
            QVERIFY(hand.value(QStringLiteral("resolved")).toBool());
            const int bet = hand.value(QStringLiteral("bet")).toInt();
            const int net = hand.value(QStringLiteral("net")).toInt();
            const QString result = hand.value(QStringLiteral("result")).toString();
            if (result == QStringLiteral("BJ"))
                QCOMPARE(net, bet * 3 / 2);          // 3:2, and doubles carry the doubled bet
            else if (result == QStringLiteral("WIN"))
                QCOMPARE(net, bet);
            else if (result == QStringLiteral("PUSH"))
                QCOMPARE(net, 0);
            else
                QCOMPARE(net, -bet);                 // LOSE or BUST
            ++handsSeen;
        }
    }
    QVERIFY(handsSeen > 0);
}

void BridgeTests::bridgeReportsCumulativeNetPerSeat() {
    BlackjackGame g;
    g.setStepInterval(0);
    g.setBotCount(1);
    g.setBet(50);
    g.stackDeck(cards({10, 10, 9, 10, 8, 7, 10}));
    g.dealRound();
    QVERIFY(g.waitingForHuman());
    g.stand();                              // dealer draws on 16 and busts
    QVERIFY(g.roundOver());
    QCOMPARE(g.netResult(), 50);

    for (const QVariant &seatValue : g.seats()) {
        const QVariantMap seat = seatValue.toMap();
        if (seat.value(QStringLiteral("human")).toBool()) {
            QCOMPARE(seat.value(QStringLiteral("net")).toInt(), g.netResult());
        } else {
            const int bankroll = seat.value(QStringLiteral("bankroll")).toInt();
            QCOMPARE(seat.value(QStringLiteral("net")).toInt(),
                     bankroll - BlackjackRules::kStartingBankroll);
            QVERIFY(seat.value(QStringLiteral("net")).toInt() > 0);
        }
    }
}

void BridgeTests::bridgeBrokePlayerMustStartOver() {
    BlackjackGame g;
    g.setStepInterval(0);
    int guard = 0;
    while (!g.isBroke() && guard++ < 500) {
        g.betMax();
        g.dealRound();
        while (!g.roundOver())
            g.canInsure() ? g.declineInsurance() : g.hit();   // busting loses fast
        g.nextRound();
    }
    QVERIFY(g.isBroke());
    QVERIFY(!g.canDeal());
    QVERIFY(g.bankroll() < 10);
    BlackjackGame reloaded;
    QVERIFY(reloaded.isBroke());
    reloaded.newGame();
    QVERIFY(!reloaded.isBroke());
    QVERIFY(reloaded.canDeal());
}

// A window without room for a third seat caps the table at two mates. A
// larger table that was already saved keeps its mates and can only shrink.
void BridgeTests::bridgeCapsTheTableWhileTheWindowIsCompact() {
    BlackjackGame g;
    g.setStepInterval(0);
    g.setBotCount(4);
    QCOMPARE(g.maxBots(), BlackjackRules::kMaxBots);
    g.setCompactLayout(true);
    QCOMPARE(g.maxBots(), 2);
    QCOMPARE(g.botCount(), 4);   // nobody is evicted
    g.setBotCount(5);
    QCOMPARE(g.botCount(), 4);   // and nobody else may sit down
    g.setBotCount(3);
    QCOMPARE(g.botCount(), 3);   // leaving stays possible, a seat at a time
    g.setBotCount(2);
    g.setBotCount(3);
    QCOMPARE(g.botCount(), 2);   // once at the cap it holds
    g.setCompactLayout(false);
    QCOMPARE(g.maxBots(), BlackjackRules::kMaxBots);
    g.setBotCount(6);
    QCOMPARE(g.botCount(), 6);
}

void BridgeTests::bridgeKeepsASavedTableBiggerThanACompactWindow() {
    {
        BlackjackGame g;
        g.setBotCount(5);
    }
    BlackjackGame reloaded;
    reloaded.setCompactLayout(true);
    QCOMPARE(reloaded.botCount(), 5);
    QCOMPARE(reloaded.maxBots(), 2);
    reloaded.newGame();
    QCOMPARE(reloaded.botCount(), 5);   // a new game reseats the same table
}

// 1, 2 and 3 stake the presets, which follow the bankroll, are only live
// while betting, and re-price themselves as the stack moves.
void BridgeTests::bridgeBetPresets() {
    BlackjackGame g;
    g.setStepInterval(0);
    g.setBotCount(0);
    QCOMPARE(g.betPresets(), QVariantList({100, 200, 500}));
    g.setBetPreset(2);
    QCOMPARE(g.bet(), 500);
    g.setBetPreset(0);
    QCOMPARE(g.bet(), 100);
    g.setBetPreset(1);
    QCOMPARE(g.bet(), 200);
    g.setBetPreset(3);
    QCOMPARE(g.bet(), 200);   // there is no fourth preset
    g.setBetPreset(-1);
    QCOMPARE(g.bet(), 200);

    // losing most of the stack re-prices the menu around what is left
    g.setBet(940);
    g.stackDeck(cards({10, 10, 6, 10}));   // 16 against 20
    playRound(g);
    g.nextRound();
    QCOMPARE(g.bankroll(), 60);
    QCOMPARE(g.betPresets(), QVariantList({10, 20, 50}));
    g.setBetPreset(2);
    QCOMPARE(g.bet(), 50);

    // and the stake is locked once the cards are out
    g.setBetPreset(0);
    g.dealRound();
    const int staked = g.bet();
    g.setBetPreset(2);
    QCOMPARE(g.bet(), staked);
}

// A bankroll down to the table minimum has one preset left, and the keys
// past the end of that list do nothing at all.
void BridgeTests::bridgeBetPresetsCollapseOnAThinBankroll() {
    BlackjackGame g;
    g.setStepInterval(0);
    g.setBotCount(0);
    g.setBet(990);
    g.stackDeck(cards({10, 10, 6, 10}));   // 16 against 20
    playRound(g);
    g.nextRound();
    QCOMPARE(g.bankroll(), 10);
    QCOMPARE(g.betPresets(), QVariantList({10}));
    g.setBetPreset(1);
    QCOMPARE(g.bet(), 10);   // clamped to the only legal bet, not to Ø 20
}
