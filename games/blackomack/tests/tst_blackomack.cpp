#include <QtTest>

#include "basicstrategy.h"
#include "blackjackrules.h"
#include "botplayer.h"
#include "cards.h"
#include "hand.h"

namespace {
Card c(int rank, Suit suit = Suit::Spades) { return {rank, suit}; }
Hand hand(std::initializer_list<int> ranks, int bet = 10) {
    Hand h;
    for (int r : ranks)
        h.cards.append(c(r));
    h.bet = bet;
    return h;
}
}

class BlackOmackTest : public QObject {
    Q_OBJECT
private slots:
    // --- Cards / hand ---
    void cardValues() {
        QCOMPARE(c(1).value(), 1);
        QCOMPARE(c(10).value(), 10);
        QCOMPARE(c(11).value(), 10);
        QCOMPARE(c(13).value(), 10);
        QCOMPARE(c(12, Suit::Hearts).rankText(), QStringLiteral("Q"));
        QVERIFY(c(12, Suit::Hearts).isRed());
        QVERIFY(!c(12, Suit::Clubs).isRed());
    }
    void handTotals() {
        QCOMPARE(hand({1, 6}).total(), 17);
        QVERIFY(hand({1, 6}).isSoft());
        QCOMPARE(hand({1, 6, 10}).total(), 17);
        QVERIFY(!hand({1, 6, 10}).isSoft());
        QCOMPARE(hand({1, 1}).total(), 12);
        QCOMPARE(hand({1, 1, 9}).total(), 21);
        QCOMPARE(hand({10, 9, 5}).total(), 24);
        QVERIFY(hand({10, 9, 5}).isBust());
        QVERIFY(!hand({10, 9}).isBust());
    }
    void blackjackVsSplitTwentyOne() {
        QVERIFY(hand({1, 13}).isBlackjack());
        QVERIFY(!hand({7, 7, 7}).isBlackjack());
        Hand split = hand({1, 13});
        split.fromSplit = true;
        QVERIFY(!split.isBlackjack());
        QCOMPARE(split.total(), 21);
    }
    void splitAndDoubleEligibility() {
        QVERIFY(hand({8, 8}).canSplit());
        QVERIFY(hand({10, 13}).canSplit());   // equal value, different rank
        QVERIFY(!hand({8, 9}).canSplit());
        QVERIFY(!hand({8, 8, 8}).canSplit());
        Hand again = hand({8, 8});
        again.fromSplit = true;
        QVERIFY(!again.canSplit());           // no re-split
        QVERIFY(hand({5, 6}).canDouble());
        QVERIFY(!hand({5, 6, 2}).canDouble());
    }
    void splitAcesFinishAfterOneCard() {
        Hand h = hand({1, 9});
        h.fromSplit = true;
        QVERIFY(h.splitAces());
        QVERIFY(h.isFinished());
        Hand tens = hand({10, 5});
        tens.fromSplit = true;
        QVERIFY(!tens.isFinished());
    }

    // --- Shoe ---
    void shoeHasSixDecks() {
        Shoe shoe(6, 42);
        QCOMPARE(shoe.remaining(), 312);
        int aces = 0;
        for (int i = 0; i < 312; ++i)
            if (shoe.draw().isAce())
                ++aces;
        QCOMPARE(aces, 24);
    }
    void shoeIsDeterministicPerSeed() {
        Shoe a(6, 7), b(6, 7), other(6, 8);
        bool differs = false;
        for (int i = 0; i < 20; ++i) {
            const Card x = a.draw();
            QVERIFY(x == b.draw());
            if (!(x == other.draw()))
                differs = true;
        }
        QVERIFY(differs);
    }
    void shoeReshuffleThreshold() {
        Shoe shoe(6, 1);
        for (int i = 0; i < 234; ++i)
            shoe.draw();
        QVERIFY(!shoe.needsShuffle());     // 78 left = exactly 25%
        shoe.draw();
        QVERIFY(shoe.needsShuffle());
        shoe.shuffle();
        QCOMPARE(shoe.remaining(), 312);
        QVERIFY(!shoe.needsShuffle());
    }

    // --- Rules ---
    void payouts() {
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
    void dealerStandsOnSoft17() {
        using namespace BlackjackRules;
        QVERIFY(!dealerShouldHit(hand({1, 6})));
        QVERIFY(dealerShouldHit(hand({1, 5})));
        QVERIFY(dealerShouldHit(hand({10, 6})));
        QVERIFY(!dealerShouldHit(hand({10, 7})));
    }
    void betValidation() {
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
    // --- Basic strategy ---
    void basicStrategySpotChecks() {
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

    // --- Bots ---
    void perfectBotFollowsBasicStrategy() {
        BotPersonality p{QStringLiteral("Zed"), 1.0, 0.0, 99};
        BotPlayer bot(p);
        Shoe shoe(6, 5);
        for (int i = 0; i < 200; ++i) {
            const Hand h = hand({shoe.draw().rank, shoe.draw().rank});
            const Card up = shoe.draw();
            QCOMPARE(bot.decide(h, up, true, true), BasicStrategy::decide(h, up, true, true));
        }
    }
    void cluelessBotDeviatesDeterministically() {
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
    void mixedSkillBotIsSeedStable() {
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
    void botBetsStayInBand() {
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
    void personalityRollIsUniqueAndLabelled() {
        QRandomGenerator rng(4);
        QStringList taken;
        for (int i = 0; i < 20; ++i) {
            const BotPersonality p = BotPersonality::roll(taken, rng);
            QVERIFY(!taken.contains(p.name));
            QVERIFY(p.skill >= 0.0 && p.skill < 1.0);
            QVERIFY(p.seed != 0);
            taken.append(p.name);
        }
        QCOMPARE(BotPersonality({QStringLiteral("x"), 0.9, 0.1, 1}).label(), QStringLiteral("cautious · sharp"));
        QCOMPARE(BotPersonality({QStringLiteral("x"), 0.5, 0.9, 1}).label(), QStringLiteral("wild · average"));
    }

    void dealerPeekCards() {
        QVERIFY(BlackjackRules::dealerPeeks(c(1)));
        QVERIFY(BlackjackRules::dealerPeeks(c(13)));
        QVERIFY(!BlackjackRules::dealerPeeks(c(9)));
    }
};

QTEST_GUILESS_MAIN(BlackOmackTest)
#include "tst_blackomack.moc"
