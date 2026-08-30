#include "ruletests.h"

#include <QtTest>

#include "basicstrategy.h"
#include "blackjackrules.h"
#include "botplayer.h"
#include "cards.h"
#include "hand.h"
#include "testhelpers.h"

void RuleTests::payouts() {
    using namespace BlackjackRules;
    QCOMPARE(payout(hand({1, 10}, 20), hand({10, 7})), 50);    // 3:2
    QCOMPARE(payout(hand({10, 9}, 20), hand({10, 7})), 40);    // 1:1
    QCOMPARE(payout(hand({10, 7}, 20), hand({10, 7})), 20);    // push
    QCOMPARE(payout(hand({10, 6}, 20), hand({10, 7})), 0);     // lose
    QCOMPARE(payout(hand({10, 6, 9}, 20), hand({10, 6, 9})), 0); // both bust: player loses
    QCOMPARE(payout(hand({10, 6}, 20), hand({10, 6, 9})), 40); // dealer busts
    QCOMPARE(payout(hand({1, 10}, 20), hand({1, 10})), 20);    // blackjack vs blackjack pushes
    QCOMPARE(payout(hand({7, 7, 7}, 20), hand({1, 10})), 0);   // 21 loses to natural
    Hand split = hand({1, 10}, 20);
    split.fromSplit = true;
    QCOMPARE(payout(split, hand({10, 7})), 40);                // split 21 pays even money
}

void RuleTests::dealerStandsOnSoft17() {
    using namespace BlackjackRules;
    QVERIFY(!dealerShouldHit(hand({1, 6})));
    QVERIFY(dealerShouldHit(hand({1, 5})));
    QVERIFY(dealerShouldHit(hand({10, 6})));
    QVERIFY(!dealerShouldHit(hand({10, 7})));
}

void RuleTests::betValidation() {
    using namespace BlackjackRules;
    QVERIFY(validBet(10, 1000));
    QVERIFY(validBet(1000, 1000));
    QVERIFY(!validBet(0, 1000));
    QVERIFY(!validBet(15, 1000));
    QVERIFY(!validBet(1010, 1000));
    QCOMPARE(clampBet(1010, 1000), 1000);
    QCOMPARE(clampBet(15, 1000), 10);
    QCOMPARE(clampBet(500, 125), 120);
    QCOMPARE(clampBet(10, 5), 0);
}

// The one-tap stakes follow the bankroll: a tenth, a quarter and a half of
// it, snapped to the {1, 2, 5} x 10^n ladder and kept inside the table's
// limits. A bankroll too small to keep them apart offers fewer.
void RuleTests::betPresetsFollowTheBankroll() {
    using BlackjackRules::betPresets;
    struct Case { int bankroll; QVector<int> presets; };
    const QVector<Case> cases = {
        {1000, {100, 200, 500}},
        {1270, {100, 200, 500}},   // 127 / 318 / 635 snap down the ladder
        {950,  {100, 200, 500}},   // 95 rounds up, 475 rounds up too
        {10000, {1000, 2000, 5000}},
        {123456, {10000, 20000, 50000}},
        {60,   {10, 20, 50}},      // 30 would repeat Ø 20, so it steps up
        {100,  {10, 20, 50}},
        {30,   {10, 20, 30}},      // the top preset can only reach the cap
        {20,   {10, 20}},
        {10,   {10}},              // nothing left to separate them with
        {19,   {10}},              // the cap is the largest legal bet, Ø 10
        {9,    {}},                // cannot even cover the table minimum
        {0,    {}},
    };
    for (const Case &test : cases) {
        QCOMPARE(betPresets(test.bankroll), test.presets);
        for (int preset : betPresets(test.bankroll))
            QVERIFY2(BlackjackRules::validBet(preset, test.bankroll),
                     qPrintable(QStringLiteral("Ø %1 of %2").arg(preset).arg(test.bankroll)));
    }
}

void RuleTests::basicStrategySpotChecks() {
    using BlackjackRules::Action;
    auto d = [](std::initializer_list<int> ranks, int up, bool dbl = true, bool split = true) {
        return BasicStrategy::decide(hand(ranks), c(up), dbl, split);
    };
    QCOMPARE(d({10, 6}, 10), Action::Hit);
    QCOMPARE(d({10, 6}, 6), Action::Stand);
    QCOMPARE(d({1, 7}, 2), Action::Stand);
    QCOMPARE(d({1, 7}, 3), Action::Double);
    QCOMPARE(d({1, 7}, 3, false), Action::Stand);    // no double → stand on soft 18
    QCOMPARE(d({1, 7}, 9), Action::Hit);
    QCOMPARE(d({8, 8}, 10), Action::Split);
    QCOMPARE(d({8, 8}, 10, true, false), Action::Hit);   // 16 when split unavailable
    QCOMPARE(d({1, 1}, 10), Action::Split);
    QCOMPARE(d({1, 1}, 10, true, false), Action::Hit);   // soft 12
    QCOMPARE(d({5, 6}, 6), Action::Double);
    QCOMPARE(d({5, 6}, 1), Action::Hit);                 // 11 vs A hits under S17
    QCOMPARE(d({5, 6}, 6, false), Action::Hit);
    QCOMPARE(d({5, 5}, 9), Action::Double);              // pair of 5s doubles as 10
    QCOMPARE(d({10, 10}, 6), Action::Stand);
    QCOMPARE(d({9, 9}, 7), Action::Stand);
    QCOMPARE(d({9, 9}, 8), Action::Split);
    QCOMPARE(d({10, 2}, 4), Action::Stand);
    QCOMPARE(d({10, 2}, 3), Action::Hit);
    QCOMPARE(d({1, 2, 3}, 5, false), Action::Hit);       // soft 16, 3 cards
    QCOMPARE(d({10, 7}, 1), Action::Stand);
}

void RuleTests::dealerPeekCards() {
    QVERIFY(BlackjackRules::dealerPeeks(c(1)));
    QVERIFY(BlackjackRules::dealerPeeks(c(13)));
    QVERIFY(!BlackjackRules::dealerPeeks(c(9)));
}

void RuleTests::perfectBotFollowsBasicStrategy() {
    BotPersonality p{QStringLiteral("Zed"), 1.0, 0.0, 99};
    BotPlayer bot(p);
    Shoe shoe(6, 5);
    for (int i = 0; i < 200; ++i) {
        const Hand h = hand({shoe.draw().rank, shoe.draw().rank});
        const Card up = shoe.draw();
        QCOMPARE(bot.decide(h, up, true, true), BasicStrategy::decide(h, up, true, true));
    }
}

void RuleTests::cluelessBotDeviatesDeterministically() {
    BotPersonality p{QStringLiteral("Moe"), 0.0, 0.2, 7};
    BotPlayer a(p), b(p);
    QCOMPARE(a.decide(hand({10, 6}), c(5), true, true), BlackjackRules::Action::Hit);   // hits 16 vs 5
    QCOMPARE(a.decide(hand({1, 5}), c(5), true, true), BlackjackRules::Action::Hit);    // never doubles soft
    QCOMPARE(a.decide(hand({1, 6}), c(5), true, true), BlackjackRules::Action::Stand);  // stands on soft 17
    QCOMPARE(a.decide(hand({8, 8}), c(10), true, true), BlackjackRules::Action::Hit);   // never splits
    QCOMPARE(a.decide(hand({10, 3}), c(10), true, true), BlackjackRules::Action::Stand); // freezes on 13
    Shoe shoe(6, 3);
    for (int i = 0; i < 100; ++i) {
        const Hand h = hand({shoe.draw().rank, shoe.draw().rank});
        const Card up = shoe.draw();
        QCOMPARE(a.decide(h, up, true, true), b.decide(h, up, true, true));
    }
}

void RuleTests::mixedSkillBotIsSeedStable() {
    BotPersonality p{QStringLiteral("Ivy"), 0.5, 0.9, 12345};
    BotPlayer a(p), b(p);
    Shoe shoe(6, 9);
    bool deviated = false;
    for (int i = 0; i < 200; ++i) {
        const Hand h = hand({shoe.draw().rank, shoe.draw().rank});
        const Card up = shoe.draw();
        const auto x = a.decide(h, up, true, true);
        QCOMPARE(x, b.decide(h, up, true, true));
        if (x != BasicStrategy::decide(h, up, true, true))
            deviated = true;
    }
    QVERIFY(deviated);
}

void RuleTests::launchSaltVariesDecisionsButNotPersonality() {
    BotPersonality p{QStringLiteral("Bea"), 0.5, 0.5, 777};
    BotPlayer unsalted(p), unsaltedAgain(p, 0), saltA(p, 0xA5A5), saltB(p, 0x5A5A);
    QCOMPARE(saltA.personality().name, p.name);
    QCOMPARE(saltA.personality().seed, p.seed);
    Shoe shoe(6, 21);
    bool aDiffersFromB = false, aDiffersFromUnsalted = false;
    for (int i = 0; i < 300; ++i) {
        const Hand h = hand({shoe.draw().rank, shoe.draw().rank});
        const Card up = shoe.draw();
        const auto base = unsalted.decide(h, up, true, true);
        QCOMPARE(base, unsaltedAgain.decide(h, up, true, true));   // salt 0 == today's behaviour
        const auto a = saltA.decide(h, up, true, true);
        const auto b = saltB.decide(h, up, true, true);
        aDiffersFromB |= a != b;
        aDiffersFromUnsalted |= a != base;
    }
    QVERIFY(aDiffersFromB);
    QVERIFY(aDiffersFromUnsalted);
}

void RuleTests::botBetsStayInBand() {
    for (double aggression : {0.0, 0.5, 1.0}) {
        BotPlayer bot({QStringLiteral("Gus"), 0.5, aggression, 3});
        const int bet = bot.chooseBet(1000);
        QCOMPARE(bet % 10, 0);
        QVERIFY(bet >= 10 && bet <= 250);
        QVERIFY(BlackjackRules::validBet(bet, 1000));
    }
    QCOMPARE(BotPlayer({QStringLiteral("Gus"), 0.5, 1.0, 3}).chooseBet(1000), 250);
    QCOMPARE(BotPlayer({QStringLiteral("Gus"), 0.5, 0.0, 3}).chooseBet(1000), 20);
    QCOMPARE(BotPlayer({QStringLiteral("Gus"), 0.5, 0.0, 3}).chooseBet(30), 10);
    QCOMPARE(BotPlayer({QStringLiteral("Gus"), 0.5, 1.0, 3}).chooseBet(5), 0);
}

void RuleTests::personalityRollIsUniqueAndLabelled() {
    QRandomGenerator rng(4);
    QStringList taken;
    for (int i = 0; i < 20; ++i) {
        const BotPersonality p = BotPersonality::roll(taken, rng);
        QVERIFY(!taken.contains(p.name));
        QVERIFY(p.skill >= 0.0 && p.skill < 1.0);
        QVERIFY(p.seed != 0);
        taken.append(p.name);
    }
    // Bets first, then play: every combination reads as plain English.
    QCOMPARE(BotPersonality({QStringLiteral("x"), 0.9, 0.1, 1}).label(), QStringLiteral("timid pro"));
    QCOMPARE(BotPersonality({QStringLiteral("x"), 0.5, 0.9, 1}).label(), QStringLiteral("bold regular"));
    QCOMPARE(BotPersonality({QStringLiteral("x"), 0.1, 0.5, 1}).label(), QStringLiteral("steady rookie"));
}
