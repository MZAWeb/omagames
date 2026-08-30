#include "besttimestests.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include "besttimes.h"
#include "savedgame.h"
#include "sudoku.h"
#include "sudokugame.h"

namespace {

constexpr quint32 kSeed = 20260829u;

TimeEntry entry(int seconds) {
    return {seconds, QDate(2026, 8, 30)};
}

QList<int> secondsIn(const BestTimes &times, Difficulty difficulty) {
    QList<int> list;
    for (const TimeEntry &e : times.entries(difficulty))
        list << e.seconds;
    return list;
}

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

void BestTimesTests::fastestFirstAndOnlyFiveKept() {
    BestTimes times;
    QVERIFY(times.entries(Difficulty::Hard).empty());
    QCOMPARE(times.best(Difficulty::Hard), 0);  // never solved

    QCOMPARE(times.insert(Difficulty::Hard, entry(300)), 0);
    QCOMPARE(times.insert(Difficulty::Hard, entry(100)), 0);  // faster, so first
    QCOMPARE(times.insert(Difficulty::Hard, entry(500)), 2);
    QCOMPARE(times.insert(Difficulty::Hard, entry(200)), 1);
    QCOMPARE(secondsIn(times, Difficulty::Hard), QList<int>({100, 200, 300, 500}));
    QCOMPARE(times.best(Difficulty::Hard), 100);

    QCOMPARE(times.insert(Difficulty::Hard, entry(400)), 3);
    QCOMPARE(secondsIn(times, Difficulty::Hard), QList<int>({100, 200, 300, 400, 500}));

    // The table is full: only a time that makes the top five gets in, and the
    // slowest falls off the end.
    QCOMPARE(times.insert(Difficulty::Hard, entry(600)), -1);
    QCOMPARE(times.insert(Difficulty::Hard, entry(150)), 1);
    QCOMPARE(secondsIn(times, Difficulty::Hard), QList<int>({100, 150, 200, 300, 400}));
    QCOMPARE(int(times.entries(Difficulty::Hard).size()), BestTimes::kMaxEntries);
}

void BestTimesTests::tiesRankBelowTheTimeAlreadyThere() {
    BestTimes times;
    times.insert(Difficulty::Easy, entry(120));
    QCOMPARE(times.insert(Difficulty::Easy, entry(120)), 1);  // a best has to be beaten
    QCOMPARE(secondsIn(times, Difficulty::Easy), QList<int>({120, 120}));
}

void BestTimesTests::tablesAreKeptPerLevel() {
    BestTimes times;
    times.insert(Difficulty::Easy, entry(60));
    times.insert(Difficulty::ExtraHard, entry(900));

    QCOMPARE(times.best(Difficulty::Easy), 60);
    QCOMPARE(times.best(Difficulty::ExtraHard), 900);
    QVERIFY(times.entries(Difficulty::Medium).empty());
    QCOMPARE(times.best(Difficulty::Medium), 0);
}

void BestTimesTests::tablesSurviveARestart() {
    {
        BestTimes times;
        times.insert(Difficulty::Medium, entry(240));
        times.insert(Difficulty::Medium, entry(180));
        times.save();
    }
    BestTimes reloaded;
    reloaded.load();
    QCOMPARE(secondsIn(reloaded, Difficulty::Medium), QList<int>({180, 240}));
    QCOMPARE(reloaded.entries(Difficulty::Medium).front().date, QDate(2026, 8, 30));

    // A file we did not write is sorted and trimmed rather than trusted.
    BestTimes wild;
    wild.fromJson(QJsonDocument::fromJson(
        "{\"easy\":[{\"seconds\":90},{\"seconds\":30},{\"seconds\":0},{\"seconds\":60},"
        "{\"seconds\":10},{\"seconds\":20},{\"seconds\":40}]}").object());
    QCOMPARE(secondsIn(wild, Difficulty::Easy), QList<int>({10, 20, 30, 40, 60}));
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
