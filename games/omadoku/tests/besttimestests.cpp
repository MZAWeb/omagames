#include "besttimestests.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include "savedgame.h"
#include "sudoku.h"
#include "sudokugame.h"

namespace {

constexpr quint32 kSeed = 20260829u;

// Solves the saved puzzle from `installSavedGame(1, kSeed)`, which leaves one
// cell open, and returns the game so the win can be inspected.
void solveTheLastCell(SudokuGame *game, const SudokuBoard &expected) {
    const int last = TestSupport::firstEmptyCell(expected);
    game->select(last);
    game->enterValue(expected.puzzle().solution[size_t(last)]);
}

}  // namespace

void BestTimesTests::initTestCase() {
    static QTemporaryDir dir;
    QVERIFY(dir.isValid());
    m_settingsDir = dir.path();
    TestSupport::redirectSettings(m_settingsDir);
}

void BestTimesTests::init() {
    QSettings settings;
    settings.clear();
    settings.sync();
}

void BestTimesTests::aWinIsRecordedWithItsRank() {
    const SudokuBoard expected = TestSupport::installSavedGame(1, kSeed);
    {
        SudokuGame game;
        QCOMPARE(game.newBestRank(), -1);
        QCOMPARE(game.bests().value(game.difficulty()).toInt(), 0);
        game.resumeSavedGame();

        QSignalSpy spy(&game, &SudokuGame::bestTimesChanged);
        solveTheLastCell(&game, expected);
        QCOMPARE(game.state(), QStringLiteral("won"));
        QCOMPARE(game.newBestRank(), 0);  // the first solve of a level is its best
        QCOMPARE(spy.count(), 1);

        // What was recorded is the elapsed clock the overlay shows.
        const QVariantList entries = game.bestTimes();
        QCOMPARE(entries.size(), 1);
        const QVariantMap best = entries.first().toMap();
        QCOMPARE(best.value(QStringLiteral("difficulty")).toString(), game.difficulty());
        QCOMPARE(best.value(QStringLiteral("label")).toString(), game.difficultyLabel());
        QCOMPARE(best.value(QStringLiteral("seconds")).toInt(), game.elapsedSeconds());
        QCOMPARE(best.value(QStringLiteral("date")).toString(), QDate::currentDate().toString(Qt::ISODate));
        QCOMPARE(game.bests().value(game.difficulty()).toInt(), game.elapsedSeconds());
    }

    // The table outlives the session, and a slower second solve ranks below.
    TestSupport::installSavedGame(1, kSeed);
    SudokuGame again;
    QCOMPARE(again.bestTimes().size(), 1);
    again.resumeSavedGame();  // the saved game carries 42 seconds already elapsed
    QCOMPARE(again.elapsedSeconds(), 42);
    solveTheLastCell(&again, expected);
    QCOMPARE(again.newBestRank(), 1);
    QCOMPARE(again.bestTimes().size(), 2);
}

void BestTimesTests::restartingOrLeavingRecordsNothing() {
    SudokuGame game;
    game.newGame(QStringLiteral("easy"));
    game.enterValue(1);
    game.restart();
    QCOMPARE(game.newBestRank(), -1);
    QVERIFY(game.bestTimes().isEmpty());

    game.backToStart();
    QCOMPARE(game.newBestRank(), -1);
    QVERIFY(game.bestTimes().isEmpty());
    QVERIFY(QSettings().value(QStringLiteral("scores/v1")).toString().isEmpty());

    // A new game after a win clears the rank, so the overlay cannot show a
    // stale "New best!" over the next puzzle.
    const SudokuBoard expected = TestSupport::installSavedGame(1, kSeed);
    SudokuGame won;
    won.resumeSavedGame();
    solveTheLastCell(&won, expected);
    QCOMPARE(won.newBestRank(), 0);
    won.newGame(QStringLiteral("easy"));
    QCOMPARE(won.newBestRank(), -1);
    QCOMPARE(won.bestTimes().size(), 1);  // the win itself is still on the table
}

// Omadoku's table refuses a solve of no time at all: it can only come from a
// hand-edited or truncated file, and it would sit at the top for ever.
void BestTimesTests::aHandEditedZeroIsNotATime() {
    QSettings settings;
    settings.setValue(QStringLiteral("scores/v1"),
                      QStringLiteral(R"({"easy":[{"seconds":0,"date":"2026-08-30"},)"
                                     R"({"seconds":95,"date":"2026-08-30"}]})"));
    settings.sync();

    SudokuGame game;
    const QVariantList entries = game.bestTimes();
    QCOMPARE(entries.size(), 1);
    QCOMPARE(entries.first().toMap().value(QStringLiteral("seconds")).toInt(), 95);
    QCOMPARE(game.bests().value(QStringLiteral("easy")).toInt(), 95);
}
