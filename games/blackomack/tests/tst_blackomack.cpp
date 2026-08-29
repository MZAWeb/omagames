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
// Pitches the opening cards one step at a time, the way the bridge does.
QVector<TableEvent> dealOut(Table &t) {
    QVector<TableEvent> events;
    while (t.phase() == Table::Phase::Dealing)
        events += t.advance();
    return events;
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

        Table table(42);
        QCOMPARE(table.shoeRemaining(), BlackjackRules::kDecks * 52);
        table.placeBets(10);
        table.advance();
        QCOMPARE(table.shoeRemaining(), BlackjackRules::kDecks * 52 - 1);
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
    void launchSaltVariesDecisionsButNotPersonality() {
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
        // Bets first, then play: every combination reads as plain English.
        QCOMPARE(BotPersonality({QStringLiteral("x"), 0.9, 0.1, 1}).label(), QStringLiteral("timid pro"));
        QCOMPARE(BotPersonality({QStringLiteral("x"), 0.5, 0.9, 1}).label(), QStringLiteral("bold regular"));
        QCOMPARE(BotPersonality({QStringLiteral("x"), 0.1, 0.5, 1}).label(), QStringLiteral("steady rookie"));
    }

    // --- Table ---
    void openingDealGoesOutOneCardAtATime() {
        Table t(1);
        t.addBot(perfect(QStringLiteral("Zed")));
        t.addBot(perfect(QStringLiteral("Mona")));       // human sits in the middle
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
        QCOMPARE(t.currentSeat(), 0);                    // the seat left of the human decides first
    }
    void naturalPaysThreeToTwo() {
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
    void dealerPeekEndsRound() {
        Table t(1);
        t.stackDeck(cards({10, 1, 9, 13}));
        t.placeBets(50);
        const auto events = dealOut(t);
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
    void splitAcesGetOneCardEach() {
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
    void splitHandsPlayInOrderAndMayDouble() {
        Table t(1);
        t.stackDeck(cards({8, 6, 8, 9, 3, 10, 10, 10}));
        t.placeBets(50);
        dealOut(t);
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
        dealOut(t);
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
        dealOut(t);
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
        QVERIFY(t.addBot(perfect(QStringLiteral("Lola"))));
        QCOMPARE(t.botCount(), 6);
        QVERIFY(!t.addBot(perfect(QStringLiteral("Nina"))));
        QCOMPARE(t.humanSeat(), 3);
        QVERIFY(t.removeLastBot());
        QCOMPARE(t.botCount(), 5);
        QVERIFY(t.human().isHuman);
        QCOMPARE(t.takenNames(), QStringList({QStringLiteral("Zed"), QStringLiteral("Mona"), QStringLiteral("Bucky"), QStringLiteral("Ivy"), QStringLiteral("Rex")}));
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
            dealOut(t);
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
    void nextRoundOnlyAfterPayout() {
        Table t(1);
        QRandomGenerator rng(1);
        QVERIFY(t.nextRound(rng).isEmpty());
        QVERIFY(t.advance().isEmpty());              // nothing automatic while betting
        t.placeBets(10);
        QVERIFY(t.nextRound(rng).isEmpty());
    }

    // --- Persistence / bridge ---
    void gameStateRoundTrips() {
        GameState state;
        state.bankroll = 1230;
        state.bestBankroll = 1480;
        state.handsPlayed = 7;
        state.netResult = 230;
        state.bots.append({{QStringLiteral("Zed"), 0.25, 0.75, 4000000000u}, 880});
        state.bots.append({{QStringLiteral("Mona"), 0.9, 0.1, 17}, 1500});
        const GameState back = GameState::fromString(state.toString());
        QCOMPARE(back.bankroll, 1230);
        QCOMPARE(back.bestBankroll, 1480);
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
        QCOMPARE(GameState::fromString(QString()).bestBankroll, 1000);
        // a save written before the high score existed keeps the stack it held
        QCOMPARE(GameState::fromString(QStringLiteral("{\"bankroll\":2400}")).bestBankroll, 2400);
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
        QCOMPARE(g.shoeRemaining(), 312);
        QCOMPARE(g.shoePercent(), 100);
        QCOMPARE(g.rulesSummary(),
                 QStringLiteral("Blackjack pays 3 to 2 · Dealer stands on all 17"));
    }
    void bridgeSeatsBotsOnAFreshTable() {
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
    void bridgeTrimsOversizedSavedTableOnLoad() {
        GameState state;
        for (int i = 0; i < 5; ++i) {
            const BotPersonality p{QStringLiteral("Saved %1").arg(i + 1), 0.5, 0.5, quint32(i + 1)};
            state.bots.append({p, 900 + i});
        }
        QSettings().setValue(QString::fromLatin1(GameState::kKey), state.toString());
        {
            BlackjackGame fiveMateTable;
            QCOMPARE(fiveMateTable.botCount(), 5);   // existing five-mate saves remain valid
        }

        for (int i = 5; i < BlackjackRules::kMaxBots + 2; ++i) {
            const BotPersonality p{QStringLiteral("Saved %1").arg(i + 1), 0.5, 0.5, quint32(i + 1)};
            state.bots.append({p, 900 + i});
        }
        QSettings().setValue(QString::fromLatin1(GameState::kKey), state.toString());
        BlackjackGame trimmed;
        QCOMPARE(trimmed.botCount(), BlackjackRules::kMaxBots);
        const GameState persisted = GameState::fromString(
            QSettings().value(QString::fromLatin1(GameState::kKey)).toString());
        QCOMPARE(persisted.bots.size(), BlackjackRules::kMaxBots);
        QCOMPARE(persisted.bots.last().personality.name, QStringLiteral("Saved 6"));
    }
    void bridgeCoachDefaultsOffAndPersists() {
        {
            BlackjackGame fresh;
            QVERIFY(!fresh.coachEnabled());
            QVERIFY(fresh.coachAction().isEmpty());
            QVERIFY(fresh.coachSituation().isEmpty());
            fresh.setCoachEnabled(true);
            QVERIFY(fresh.coachEnabled());
        }
        {
            BlackjackGame restored;
            QVERIFY(restored.coachEnabled());
            restored.toggleCoach();
            QVERIFY(!restored.coachEnabled());
        }
        BlackjackGame restoredOff;
        QVERIFY(!restoredOff.coachEnabled());
    }
    void bridgeCoachDescribesKnownHandsOnlyOnYourTurn() {
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
    void bridgeCoachSaysNothingWhileItIsOff() {
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
    void bridgeSkipPacingStopsBeforeTheHumanDecision() {
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
    void bridgeLocksTheStakeOnceDealt() {
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
            g.stand();
        g.setBet(80);                // still locked while the result is on the table
        QCOMPARE(g.bet(), 50);
        g.nextRound();
        g.setBet(80);
        QCOMPARE(g.bet(), 80);
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
    // The payout screen reads its amounts straight off the model.
    void bridgeReportsWhatEachHandPaid() {
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
    void bridgeReportsCumulativeNetPerSeat() {
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
    // The best bankroll ever held is a high score: only a bigger stack raises
    // it, and starting over does not clear it.
    void bridgeBestBankrollIsAHighScore() {
        int best = 0;
        {
            BlackjackGame g;
            g.setStepInterval(0);
            g.setBotCount(0);
            g.setBet(50);
            QCOMPARE(g.bestBankroll(), 1000);   // the opening stake is the first record
            QVERIFY(!g.newBest());

            // player 19 against the dealer's 17: a payout that beats the record
            g.stackDeck(cards({10, 10, 9, 7}));
            playRound(g);
            QCOMPARE(g.bankroll(), 1050);
            QCOMPARE(g.bestBankroll(), 1050);
            QVERIFY(g.newBest());
            g.nextRound();
            QVERIFY(!g.newBest());              // celebrated for exactly one round
            QCOMPARE(g.bestBankroll(), 1050);

            // 16 against 17: a loss leaves the record where it was
            g.stackDeck(cards({10, 10, 6, 7}));
            playRound(g);
            QCOMPARE(g.bankroll(), 1000);
            QCOMPARE(g.bestBankroll(), 1050);
            QVERIFY(!g.newBest());
            g.nextRound();

            // 17 against 17: a push returns the bet and sets no record
            g.stackDeck(cards({10, 10, 7, 7}));
            playRound(g);
            QCOMPARE(g.bankroll(), 1000);
            QVERIFY(!g.newBest());
            QCOMPARE(g.bestBankroll(), 1050);
            g.nextRound();

            // back to the record, which is matched rather than beaten
            g.stackDeck(cards({10, 10, 9, 7}));
            playRound(g);
            QCOMPARE(g.bankroll(), 1050);
            QVERIFY(!g.newBest());
            QCOMPARE(g.bestBankroll(), 1050);
            g.nextRound();

            g.newGame();
            QCOMPARE(g.bankroll(), 1000);
            QCOMPARE(g.bestBankroll(), 1050);   // a high score outlives the game
            QVERIFY(!g.newBest());
            best = g.bestBankroll();
        }
        BlackjackGame relaunched;
        QCOMPARE(relaunched.bestBankroll(), best);
        QVERIFY(!relaunched.newBest());
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
