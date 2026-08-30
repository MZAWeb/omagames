#include "leveltests.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include "savedgame.h"
#include "sudokugame.h"
#include "sudokugrader.h"
#include "sudokulevels.h"

void LevelTests::initTestCase() {
    // Never touch the real ~/.config/Omacom while testing.
    static QTemporaryDir dir;
    QVERIFY(dir.isValid());
    m_settingsDir = dir.path();
    TestSupport::redirectSettings(m_settingsDir);
}

void LevelTests::init() {
    QSettings settings;
    settings.clear();
    settings.sync();
}

void LevelTests::idsRoundTripAndSurviveNonsense() {
    for (int i = 0; i < kDifficultyCount; ++i) {
        const Difficulty level = Difficulty(i);
        Difficulty back = Difficulty::ExtraHard;
        QVERIFY(SudokuLevels::fromId(SudokuLevels::id(level), &back));
        QCOMPARE(int(back), i);
        QVERIFY(!SudokuLevels::label(SudokuLevels::id(level)).isEmpty());
    }

    // An id no level answers to leaves the caller's choice alone, so a stale
    // setting cannot silently change the level.
    Difficulty untouched = Difficulty::Hard;
    QVERIFY(!SudokuLevels::fromId(QStringLiteral("nonsense"), &untouched));
    QCOMPARE(int(untouched), int(Difficulty::Hard));
    QVERIFY(SudokuLevels::label(QStringLiteral("nonsense")).isEmpty());
}

void LevelTests::everyLevelIntroducesRungsOfItsOwn() {
    const QVariantList levels = SudokuLevels::all();
    QCOMPARE(levels.size(), kDifficultyCount);

    // Every rung of the ladder is introduced by exactly one level, so the
    // start screen promises each technique once and misses none.
    QStringList seen;
    for (int i = 0; i < levels.size(); ++i) {
        const QVariantMap level = levels.at(i).toMap();
        QCOMPARE(level.value(QStringLiteral("id")).toString(), SudokuLevels::id(Difficulty(i)));
        QVERIFY(!level.value(QStringLiteral("description")).toString().isEmpty());
        const QStringList techniques = level.value(QStringLiteral("techniques")).toStringList();
        QVERIFY(!techniques.isEmpty());
        for (const QString &technique : techniques) {
            QVERIFY2(!seen.contains(technique), qPrintable(technique + QStringLiteral(" listed twice")));
            seen << technique;
        }
    }
    QCOMPARE(seen.size(), SudokuGrader::kTechniqueCount);

    // The names are the ladder in order, easiest rung first.
    QCOMPARE(SudokuLevels::techniqueName(SudokuGrader::Technique::LastDigit),
             QStringLiteral("Last digit"));
    QCOMPARE(seen.first(), QStringLiteral("Last digit"));
    QCOMPARE(seen.last(), SudokuLevels::techniqueName(SudokuGrader::kHardestTechnique));
}

void LevelTests::theTimesTableRanksEachLevelFastestFirst() {
    OmaGames::ScoreTable table = SudokuLevels::timesTable();
    QCOMPARE(table.maxEntries(), 5);
    const QDate today(2026, 8, 30);
    for (int i = 0; i < kDifficultyCount; ++i) {
        const QString id = SudokuLevels::id(Difficulty(i));
        QCOMPARE(table.insert(id, {300, today, {}}), 0);
        QCOMPARE(table.insert(id, {120, today, {}}), 0);  // faster is better
        QCOMPARE(table.insert(id, {400, today, {}}), 2);
        QCOMPARE(table.best(id), 120);
        QCOMPARE(table.toVariantList(id).size(), 3);
    }
    // Each level keeps its own table, and only the four of them exist.
    QCOMPARE(table.insert(QStringLiteral("nonsense"), {60, today, {}}), -1);
    QCOMPARE(table.best(SudokuLevels::id(Difficulty::Easy)), 120);

    // A solve of no time at all is a hand-edited file, not a record, so it
    // does not survive the trip through storage.
    OmaGames::ScoreTable edited = SudokuLevels::timesTable();
    QVERIFY(edited.insert(SudokuLevels::id(Difficulty::Easy), {0, today, {}}) >= 0);
    OmaGames::ScoreTable read = SudokuLevels::timesTable();
    read.fromJson(edited.toJson());
    QCOMPARE(read.toVariantList(SudokuLevels::id(Difficulty::Easy)).size(), 0);
}

void LevelTests::difficultiesReachQmlWithLabels() {
    SudokuGame game;
    const QVariantList levels = game.difficulties();
    QCOMPARE(levels, SudokuLevels::all());

    const QVariantMap extra = levels.last().toMap();
    QCOMPARE(extra.value(QStringLiteral("id")).toString(), QStringLiteral("extrahard"));
    game.newGame(extra.value(QStringLiteral("id")).toString());
    QCOMPARE(game.difficulty(), QStringLiteral("extrahard"));
    QCOMPARE(game.difficultyLabel(), extra.value(QStringLiteral("label")).toString());
    // What the puzzle needs is one of the rungs the level promises.
    QVERIFY(extra.value(QStringLiteral("techniques")).toStringList().contains(game.techniqueLabel()));

    game.newGame(QStringLiteral("hard"));
    QCOMPARE(game.difficulty(), QStringLiteral("hard"));
    game.newGame(QStringLiteral("nonsense"));  // an unknown id lands on Easy
    QCOMPARE(game.difficulty(), QStringLiteral("easy"));
}
