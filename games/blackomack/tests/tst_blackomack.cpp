#include <QtTest>

#include "basicstrategy.h"
#include "blackjackgame.h"
#include "gamestate.h"
#include "blackjackrules.h"
#include "botplayer.h"
#include "cards.h"
#include "hand.h"
#include "table.h"

namespace {
Card c(int rank, Suit suit = Suit::Spades) { return {rank, suit}; }
Hand hand(std::initializer_list<int> ranks, int bet = 10) {
    Hand h;
    for (int r : ranks)
        h.cards.append(c(r));
    h.bet = bet;
    return h;
}
QVector<Card> cards(std::initializer_list<int> ranks) {
    QVector<Card> v;
    for (int r : ranks)
        v.append(c(r));
    return v;
}
// Runs automatic steps until the human must act or the round is settled.
void autoplay(Table &t) {
    while (!t.waitingForHuman() && !t.roundOver() && t.phase() != Table::Phase::Betting)
        t.advance();
}
BotPersonality perfect(const QString &name, quint32 seed = 1) { return {name, 1.0, 0.5, seed}; }
}

class BlackOmackTest : public QObject {
    Q_OBJECT
    QTemporaryDir m_settingsDir;

    // Plays the human's hands by the book until the round is settled.
    static void playRound(BlackjackGame &g) {
        g.dealRound();
        while (!g.roundOver()) {
            QVERIFY(g.waitingForHuman());
            if (g.canStand())
                g.stand();
        }
    }

private slots:
    void initTestCase() {
        QVERIFY(m_settingsDir.isValid());
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QCoreApplication::setOrganizationName(QStringLiteral("Omacom"));
        QCoreApplication::setApplicationName(QStringLiteral("blackomack-test"));
    }
    void cleanup() {
        QSettings().clear();
    }

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

    // --- Table ---
    void naturalPaysThreeToTwo() {
        Table t(1);
        t.stackDeck(cards({1, 7, 13, 10}));
        QCOMPARE(t.placeBets(50).size(), 1);
        QCOMPARE(t.human().bankroll, 950);
        t.deal();
        QCOMPARE(t.phase(), Table::Phase::DealerTurn);   // nothing for the human to decide
        QVERIFY(t.holeHidden());
        autoplay(t);
        QVERIFY(t.roundOver());
        QVERIFY(!t.holeHidden());
        QCOMPARE(t.dealer().cards.size(), 2);            // no draw with nobody to beat
        QCOMPARE(t.human().hands[0].returned, 125);
        QCOMPARE(t.human().bankroll, 1075);
    }
    void dealerPeekEndsRound() {
        Table t(1);
        t.stackDeck(cards({10, 1, 9, 13}));
        t.placeBets(50);
        const auto events = t.deal();
        QCOMPARE(t.phase(), Table::Phase::Payout);
        QVERIFY(!t.holeHidden());
        QCOMPARE(events.last().type, TableEvent::DealerBlackjack);
        QVERIFY(t.act(Table::Action::Hit).isEmpty());
        autoplay(t);
        QCOMPARE(t.human().bankroll, 950);
        QCOMPARE(t.human().hands[0].returned, 0);
    }
    void doubleTakesExactlyOneCard() {
        Table t(1);
        t.stackDeck(cards({5, 6, 6, 10, 10, 9}));
        t.placeBets(100);
        t.deal();
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
    void splitAcesGetOneCardEach() {
        Table t(1);
        t.stackDeck(cards({1, 9, 1, 7, 10, 5, 2}));
        t.placeBets(50);
        t.deal();
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
    void splitHandsPlayInOrderAndMayDouble() {
        Table t(1);
        t.stackDeck(cards({8, 6, 8, 9, 3, 10, 10, 10}));
        t.placeBets(50);
        t.deal();
        t.act(Table::Action::Split);
        QCOMPARE(t.currentHand(), 0);
        QVERIFY(t.canAct(t.humanSeat(), Table::Action::Double));   // double after split
        QVERIFY(!t.canAct(t.humanSeat(), Table::Action::Split));   // no re-split
        t.act(Table::Action::Double);
        QCOMPARE(t.currentHand(), 1);
        QVERIFY(t.waitingForHuman());
        t.act(Table::Action::Stand);
        autoplay(t);
        QVERIFY(t.dealer().isBust());
        QCOMPARE(t.human().bankroll, 1000 + 100 + 50);
    }
    void dealerStandsOnSoft17AtTheTable() {
        Table t(1);
        t.stackDeck(cards({10, 1, 8, 6}));
        t.placeBets(10);
        t.deal();
        t.act(Table::Action::Stand);
        autoplay(t);
        QCOMPARE(t.dealer().cards.size(), 2);
        QCOMPARE(t.dealer().total(), 17);
        QCOMPARE(t.human().bankroll, 1010);
    }
    void doubleAndSplitNeedFunds() {
        Table t(1);
        t.setHumanBankroll(60);
        t.stackDeck(cards({8, 6, 8, 9}));
        t.placeBets(50);
        t.deal();
        QVERIFY(t.waitingForHuman());
        QVERIFY(!t.canAct(t.humanSeat(), Table::Action::Double));
        QVERIFY(!t.canAct(t.humanSeat(), Table::Action::Split));
        QVERIFY(t.act(Table::Action::Split).isEmpty());
        QVERIFY(t.canAct(t.humanSeat(), Table::Action::Hit));
    }
    void seatingKeepsHumanInTheMiddle() {
        Table t(1);
        QCOMPARE(t.humanSeat(), 0);
        QVERIFY(t.addBot(perfect(QStringLiteral("Zed"))));
        QCOMPARE(t.humanSeat(), 0);
        QVERIFY(t.addBot(perfect(QStringLiteral("Mona"))));
        QCOMPARE(t.humanSeat(), 1);
        t.addBot(perfect(QStringLiteral("Bucky")));
        t.addBot(perfect(QStringLiteral("Ivy")));
        t.addBot(perfect(QStringLiteral("Rex")));
        QCOMPARE(t.botCount(), 5);
        QVERIFY(!t.addBot(perfect(QStringLiteral("Lola"))));
        QCOMPARE(t.humanSeat(), 2);
        QVERIFY(t.removeLastBot());
        QCOMPARE(t.botCount(), 4);
        QVERIFY(t.human().isHuman);
        QCOMPARE(t.takenNames(), QStringList({QStringLiteral("Zed"), QStringLiteral("Mona"), QStringLiteral("Bucky"), QStringLiteral("Ivy")}));
    }
    void fullRoundsWithBotsBalanceTheBooks() {
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
            t.deal();
            while (!t.roundOver()) {
                autoplay(t);
                if (t.waitingForHuman())
                    t.act(BasicStrategy::decide(t.human().hands[t.currentHand()], t.dealerUpCard(),
                                                t.canAct(t.humanSeat(), Table::Action::Double),
                                                t.canAct(t.humanSeat(), Table::Action::Split)));
            }
            for (int i = 0; i < t.seats().size(); ++i) {
                int staked = 0, returned = 0;
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
    void brokeBotIsReplacedNextRound() {
        Table t(3);
        QRandomGenerator rng(9);
        t.addBot(perfect(QStringLiteral("Zed")), 5);
        t.placeBets(10);
        QVERIFY(t.seats()[1].hands.isEmpty());     // can't afford the minimum: sits out
        t.deal();
        while (!t.roundOver()) {
            autoplay(t);
            if (t.waitingForHuman())
                t.act(Table::Action::Stand);
        }
        const auto events = t.nextRound(rng);
        QCOMPARE(events.size(), 2);
        QCOMPARE(events[0].type, TableEvent::BotLeft);
        QCOMPARE(events[1].type, TableEvent::BotJoined);
        QVERIFY(t.seats()[1].bot.personality().name != QStringLiteral("Zed"));
        QCOMPARE(t.seats()[1].bankroll, 1000);
    }
    void nextRoundOnlyAfterPayout() {
        Table t(1);
        QRandomGenerator rng(1);
        QVERIFY(t.nextRound(rng).isEmpty());
        t.placeBets(10);
        QVERIFY(t.nextRound(rng).isEmpty());
        QVERIFY(t.advance().isEmpty());              // nothing automatic before the deal
    }

    // --- Persistence / bridge ---
    void gameStateRoundTrips() {
        GameState state;
        state.bankroll = 1230;
        state.handsPlayed = 7;
        state.netResult = 230;
        state.bots.append({{QStringLiteral("Zed"), 0.25, 0.75, 4000000000u}, 880});
        state.bots.append({{QStringLiteral("Mona"), 0.9, 0.1, 17}, 1500});
        const GameState back = GameState::fromString(state.toString());
        QCOMPARE(back.bankroll, 1230);
        QCOMPARE(back.handsPlayed, 7);
        QCOMPARE(back.netResult, 230);
        QCOMPARE(back.bots.size(), 2);
        QCOMPARE(back.bots[0].personality.name, QStringLiteral("Zed"));
        QCOMPARE(back.bots[0].personality.skill, 0.25);
        QCOMPARE(back.bots[0].personality.aggression, 0.75);
        QCOMPARE(back.bots[0].personality.seed, 4000000000u);
        QCOMPARE(back.bots[0].bankroll, 880);
        QCOMPARE(back.bots[1].bankroll, 1500);
        QCOMPARE(GameState::fromString(QString()).bankroll, 1000);   // fresh install
    }
    void bridgeBetting() {
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
    }
    void bridgePlaysAndPersists() {
        {
            BlackjackGame g;
            g.setStepInterval(0);
            g.setBotCount(3);
            QCOMPARE(g.botCount(), 3);
            QCOMPARE(g.seats().size(), 4);
            QCOMPARE(g.humanSeat(), 1);
            g.setBet(20);
            for (int i = 0; i < 5; ++i) {
                playRound(g);
                QVERIFY(!g.canDeal());
                QCOMPARE(g.phase(), QStringLiteral("payout"));
                QVERIFY(!g.dealerHand().value(QStringLiteral("cards")).toList().isEmpty());
                g.nextRound();
                QCOMPARE(g.phase(), QStringLiteral("betting"));
            }
            QCOMPARE(g.handsPlayed(), 5);
            QCOMPARE(g.netResult(), g.bankroll() - 1000);
            g.setBotCount(1);
            QCOMPARE(g.botCount(), 1);
        }
        BlackjackGame again;
        QCOMPARE(again.botCount(), 1);
        QCOMPARE(again.handsPlayed(), 5);
        QVERIFY(again.bankroll() != 0);
        const QVariantMap bot = again.seats().at(again.humanSeat() == 0 ? 1 : 0).toMap();
        QVERIFY(!bot.value(QStringLiteral("personality")).toString().isEmpty());
        QVERIFY(!bot.value(QStringLiteral("human")).toBool());
        again.newGame();
        QCOMPARE(again.bankroll(), 1000);
        QCOMPARE(again.handsPlayed(), 0);
        QCOMPARE(again.botCount(), 1);
        QVERIFY(!again.isBroke());
    }
    void bridgeBrokePlayerMustStartOver() {
        BlackjackGame g;
        g.setStepInterval(0);
        int guard = 0;
        while (!g.isBroke() && guard++ < 500) {
            g.betMax();
            g.dealRound();
            while (!g.roundOver())
                g.hit();                     // hitting until bust loses fast
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

    void dealerPeekCards() {
        QVERIFY(BlackjackRules::dealerPeeks(c(1)));
        QVERIFY(BlackjackRules::dealerPeeks(c(13)));
        QVERIFY(!BlackjackRules::dealerPeeks(c(9)));
    }
};

QTEST_GUILESS_MAIN(BlackOmackTest)
#include "tst_blackomack.moc"
