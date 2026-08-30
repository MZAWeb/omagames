#include "persistencetests.h"

#include <QtTest>

#include "blackjackgame.h"
#include "blackjackrules.h"
#include "botplayer.h"
#include "gamestate.h"
#include "seatlayout.h"
#include "testhelpers.h"

void PersistenceTests::cleanup() {
    QSettings().clear();
}

void PersistenceTests::gameStateRoundTrips() {
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

void PersistenceTests::bridgeTrimsOversizedSavedTableOnLoad() {
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

// A saved table comes back whole and in its place round the felt: every
// mate keeps its identity and the order it was saved in, and the human
// still sits where the sweep reaches the bottom of the oval.
void PersistenceTests::bridgeLoadsSavedTableInPlayOrder() {
    QSettings().clear();
    GameState state;
    const QStringList names{QStringLiteral("Zed"), QStringLiteral("Mona"),
                            QStringLiteral("Bucky"), QStringLiteral("Ivy")};
    for (int i = 0; i < names.size(); ++i)
        state.bots.append({{names[i], 0.5, 0.5, quint32(i + 1)}, 900 + i});
    QSettings().setValue(QString::fromLatin1(GameState::kKey), state.toString());

    BlackjackGame g;
    QCOMPARE(g.botCount(), names.size());
    QCOMPARE(g.humanSeat(), SeatLayout::matesBeforeHuman(names.size()));
    QStringList seated;
    for (int i = 0, mate = 0; i < g.seats().size(); ++i) {
        const QVariantMap seat = g.seats().at(i).toMap();
        if (seat.value(QStringLiteral("human")).toBool()) {
            QCOMPARE(i, g.humanSeat());
            continue;
        }
        QCOMPARE(seat.value(QStringLiteral("bankroll")).toInt(), 900 + mate++);
        seated.append(seat.value(QStringLiteral("name")).toString());
    }
    QCOMPARE(seated, names);

    g.setBotCount(3);                       // sending one home rewrites the rest in order
    const GameState written = GameState::fromString(
        QSettings().value(QString::fromLatin1(GameState::kKey)).toString());
    QStringList savedAgain;
    for (const GameState::Bot &b : written.bots)
        savedAgain.append(b.personality.name);
    QCOMPARE(savedAgain, QStringList(names.mid(0, 3)));
}

void PersistenceTests::bridgeCoachDefaultsOffAndPersists() {
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

void PersistenceTests::bridgePlaysAndPersists() {
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

// The best bankroll ever held is a high score: only a bigger stack raises
// it, and starting over does not clear it.
void PersistenceTests::bridgeBestBankrollIsAHighScore() {
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
