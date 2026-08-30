#include "bridgetests.h"

#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
#include <deque>

#include "omasnakegame.h"

namespace {

constexpr quint32 kSeed = 20260830u;
// Out of the way of every scripted snake, so a scenario's dot stays put.
constexpr QPoint kParked {1, 1};

std::deque<QPoint> row(QPoint head, int length) {
    std::deque<QPoint> body;
    for (int i = 0; i < length; ++i)
        body.push_back(head - QPoint(i, 0));
    return body;
}

// A game past its opening beat, with the dot parked far from the snake.
void quietStart(OmasnakeGame &game, Mode mode = Mode::Classic, Difficulty difficulty = Difficulty::Normal) {
    game.setStepInterval(0);
    game.startGame(mode, difficulty, kSeed);
    for (int i = 0; i < Game::kReadyTicks; ++i)
        game.step();
    game.engineForTests()->placeFood(kParked);
}

void moves(OmasnakeGame &game, int count) {
    for (int m = 0; m < count; ++m) {
        const int ticks = game.engine()->moveTicks();
        for (int i = 0; i < ticks; ++i)
            game.step();
    }
}

}  // namespace

void BridgeTests::initTestCase() {
    static QTemporaryDir dir;
    QVERIFY(dir.isValid());
    m_settingsDir = dir.path();
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir);
}

void BridgeTests::init() {
    QSettings settings;
    settings.clear();
    settings.sync();
}

void BridgeTests::startsOnTheStartScreenWithModesAndDifficulties() {
    OmasnakeGame game;
    QCOMPARE(game.phase(), QStringLiteral("start"));
    QVERIFY(!game.engine());
    QCOMPARE(game.score(), 0);
    QCOMPARE(game.length(), 0);
    QCOMPARE(game.best(), 0);
    QVERIFY(game.gameOverReason().isEmpty());
    QCOMPARE(game.fieldWidth(), 32);
    QCOMPARE(game.fieldHeight(), 24);

    QCOMPARE(game.modes().size(), 2);
    QCOMPARE(game.modes().at(0).toMap().value(QStringLiteral("id")).toString(), QStringLiteral("classic"));
    QCOMPARE(game.modes().at(1).toMap().value(QStringLiteral("label")).toString(), QStringLiteral("Wrap"));
    // The edges switch shows one line under it, taken from the chosen mode, so
    // the two lines have to exist and have to say different things.
    const QString classicText = game.modes().at(0).toMap().value(QStringLiteral("description")).toString();
    const QString wrapText = game.modes().at(1).toMap().value(QStringLiteral("description")).toString();
    QVERIFY(!classicText.isEmpty());
    QVERIFY(!wrapText.isEmpty());
    QVERIFY(classicText != wrapText);
    QCOMPARE(game.mode(), QStringLiteral("classic"));
    QCOMPARE(game.modeLabel(), QStringLiteral("Classic"));

    QCOMPARE(game.difficulties().size(), 3);
    QCOMPARE(game.difficulties().at(0).toMap().value(QStringLiteral("id")).toString(), QStringLiteral("slow"));
    QCOMPARE(game.difficulties().at(2).toMap().value(QStringLiteral("label")).toString(), QStringLiteral("Fast"));
    QVERIFY(game.difficulties().at(1).toMap().value(QStringLiteral("description")).toString().contains(QStringLiteral("6.7")));
    QCOMPARE(game.difficulty(), QStringLiteral("normal"));

    QVERIFY(game.highScores().isEmpty());
    QCOMPARE(game.bests().size(), kModeCount * kDifficultyCount);
    QCOMPARE(game.bests().value(QStringLiteral("wrap-fast")).toInt(), 0);
}

void BridgeTests::newGameExposesEngineState() {
    OmasnakeGame game;
    game.setStepInterval(0);
    QCOMPARE(game.stepInterval(), 0);
    QSignalSpy phases(&game, &OmasnakeGame::phaseChanged);
    game.newGame(QStringLiteral("fast"));
    QCOMPARE(phases.count(), 1);
    QCOMPARE(game.phase(), QStringLiteral("playing"));
    QCOMPARE(game.difficulty(), QStringLiteral("fast"));
    QCOMPARE(game.difficultyLabel(), QStringLiteral("Fast"));
    QCOMPARE(game.mode(), QStringLiteral("classic"));
    QCOMPARE(game.score(), 0);
    QCOMPARE(game.length(), Game::kStartLength);
    QCOMPARE(game.multiplier(), 1);
    QCOMPARE(game.cellsPerSecond(), double(Rules::kTicksPerSecond) / Rules::params(Difficulty::Fast).startMoveTicks);
    QCOMPARE(game.newHighScoreRank(), -1);
    QVERIFY(game.engine());

    // An unknown id falls back to the difficulty in play.
    game.newGame(QStringLiteral("blistering"));
    QCOMPARE(game.difficulty(), QStringLiteral("fast"));

    game.backToStart();
    QCOMPARE(game.phase(), QStringLiteral("start"));
    QVERIFY(!game.engine());
}

void BridgeTests::turnsAndPauseGoThroughTheBridge() {
    OmasnakeGame game;
    quietStart(game, Mode::Wrap);
    const QPoint start = game.engine()->snake().head();

    QSignalSpy frames(&game, &OmasnakeGame::frameChanged);
    game.turn(QStringLiteral("left"));  // a reversal, ignored
    game.turn(QStringLiteral("nowhere"));
    game.turn(QStringLiteral("up"));
    moves(game, 1);
    QCOMPARE(game.engine()->snake().head(), start + QPoint(0, -1));
    QCOMPARE(frames.count(), game.engine()->moveTicks());

    QSignalSpy paused(&game, &OmasnakeGame::pausedChanged);
    game.togglePause();
    QVERIFY(game.paused());
    QCOMPARE(paused.count(), 1);
    const QPoint frozen = game.engine()->snake().head();
    moves(game, 5);
    QCOMPARE(game.engine()->snake().head(), frozen);
    game.togglePause();
    QVERIFY(!game.paused());
    QCOMPARE(paused.count(), 2);
    moves(game, 1);
    QCOMPARE(game.engine()->snake().head(), frozen + QPoint(0, -1));
}

void BridgeTests::readyBeatIsExposed() {
    OmasnakeGame game;
    game.setStepInterval(0);
    QSignalSpy ready(&game, &OmasnakeGame::readyChanged);
    game.startGame(Mode::Classic, Difficulty::Slow, kSeed);
    QVERIFY(game.ready());
    QCOMPARE(ready.count(), 1);
    for (int i = 0; i < Game::kReadyTicks; ++i)
        game.step();
    QVERIFY(!game.ready());
    QCOMPARE(ready.count(), 2);
}

void BridgeTests::eatingReportsScoreLengthAndSpeed() {
    OmasnakeGame game;
    quietStart(game, Mode::Wrap);
    game.engineForTests()->placeSnake(row({3, 12}, Game::kStartLength), Direction::Right);

    QSignalSpy scored(&game, &OmasnakeGame::scored);
    QSignalSpy speedUps(&game, &OmasnakeGame::spedUp);
    QSignalSpy lengths(&game, &OmasnakeGame::lengthChanged);
    QSignalSpy scores(&game, &OmasnakeGame::scoreChanged);
    for (int i = 0; i < Game::kFoodsPerBonus; ++i) {
        game.engineForTests()->placeFood(game.engine()->snake().head() + QPoint(1, 0));
        moves(game, 1);
    }
    QCOMPARE(scored.count(), Game::kFoodsPerBonus);
    QCOMPARE(scored.first().at(0).toString(), QStringLiteral("+10"));
    QCOMPARE(scored.first().at(1).toInt(), 4);
    QCOMPARE(scored.first().at(2).toInt(), 12);
    QCOMPARE(lengths.count(), Game::kFoodsPerBonus);
    QCOMPARE(scores.count(), Game::kFoodsPerBonus);
    QCOMPARE(game.length(), Game::kStartLength + Game::kFoodsPerBonus);
    QCOMPARE(game.score(), Game::kFoodsPerBonus * Game::kFoodScore);
    QCOMPARE(speedUps.count(), 1);
    QCOMPARE(game.cellsPerSecond(), double(Rules::kTicksPerSecond) / (Rules::params(Difficulty::Normal).startMoveTicks - 1));

    // The bonus that follows the fifth food pays its flat fifty.
    QVERIFY(game.engine()->hasBonus());
    game.engineForTests()->placeBonus(game.engine()->snake().head() + QPoint(1, 0), Game::kBonusLifetimeTicks);
    game.engineForTests()->placeFood(kParked);
    moves(game, 1);
    QCOMPARE(scored.count(), Game::kFoodsPerBonus + 1);
    QCOMPARE(scored.last().at(0).toString(), QStringLiteral("Bonus +50"));
    QCOMPARE(game.score(), Game::kFoodsPerBonus * Game::kFoodScore + Game::kBonusScore);
}

void BridgeTests::scriptedGameOverRecordsAHighScore() {
    OmasnakeGame game;
    quietStart(game);
    game.engineForTests()->placeSnake(row({3, 12}, Game::kStartLength), Direction::Right);
    for (int i = 0; i < 3; ++i) {
        game.engineForTests()->placeFood(game.engine()->snake().head() + QPoint(1, 0));
        moves(game, 1);
    }
    const int score = game.score();
    const int length = game.length();
    QCOMPARE(score, 3 * Game::kFoodScore);

    QSignalSpy crashes(&game, &OmasnakeGame::crashed);
    QSignalSpy tables(&game, &OmasnakeGame::highScoresChanged);
    game.engineForTests()->placeFood(kParked);
    game.engineForTests()->placeSnake(row({Game::kWidth - 1, 5}, length), Direction::Right);
    moves(game, 1);

    QCOMPARE(crashes.count(), 1);
    QCOMPARE(game.phase(), QStringLiteral("gameover"));
    QCOMPARE(game.gameOverReason(), QStringLiteral("wall"));
    QCOMPARE(game.newHighScoreRank(), 0);
    QCOMPARE(tables.count(), 1);
    QCOMPARE(game.best(), score);
    QCOMPARE(game.bests().value(QStringLiteral("classic-normal")).toInt(), score);
    QCOMPARE(game.highScores().size(), 1);
    const QVariantMap entry = game.highScores().first().toMap();
    QCOMPARE(entry.value(QStringLiteral("score")).toInt(), score);
    QCOMPARE(entry.value(QStringLiteral("length")).toInt(), length);
    QCOMPARE(entry.value(QStringLiteral("table")).toString(), QStringLiteral("classic-normal"));
    QCOMPARE(entry.value(QStringLiteral("date")).toString(), QDate::currentDate().toString(Qt::ISODate));

    // Nothing moves after the end.
    moves(game, 5);
    QCOMPARE(game.phase(), QStringLiteral("gameover"));
    QCOMPARE(game.score(), score);
}

void BridgeTests::selfCrashAndWrapModeAreReported() {
    OmasnakeGame wrap;
    quietStart(wrap, Mode::Wrap);
    wrap.engineForTests()->placeSnake(row({Game::kWidth - 1, 5}, Game::kStartLength), Direction::Right);
    moves(wrap, 1);
    QCOMPARE(wrap.phase(), QStringLiteral("playing"));
    QCOMPARE(wrap.engine()->snake().head(), QPoint(0, 5));

    wrap.engineForTests()->placeSnake({{10, 10}, {10, 11}, {11, 11}, {11, 10}, {12, 10}}, Direction::Right);
    moves(wrap, 1);
    QCOMPARE(wrap.phase(), QStringLiteral("gameover"));
    QCOMPARE(wrap.gameOverReason(), QStringLiteral("self"));
    QCOMPARE(wrap.bests().value(QStringLiteral("wrap-normal")).toInt(), 0);
}

void BridgeTests::restartKeepsModeAndDifficulty() {
    OmasnakeGame game;
    quietStart(game, Mode::Wrap, Difficulty::Fast);
    game.engineForTests()->placeSnake(row({3, 12}, Game::kStartLength), Direction::Right);
    game.engineForTests()->placeFood(game.engine()->snake().head() + QPoint(1, 0));
    moves(game, 1);
    QCOMPARE(game.score(), Game::kFoodScore);

    game.pause();
    game.restart();
    QVERIFY(!game.paused());
    QVERIFY(game.ready());
    QCOMPARE(game.score(), 0);
    QCOMPARE(game.length(), Game::kStartLength);
    QCOMPARE(game.mode(), QStringLiteral("wrap"));
    QCOMPARE(game.difficulty(), QStringLiteral("fast"));
    QCOMPARE(game.phase(), QStringLiteral("playing"));
}

void BridgeTests::modeOnlyChangesBetweenGames() {
    OmasnakeGame game;
    game.setStepInterval(0);
    QSignalSpy modes(&game, &OmasnakeGame::modeChanged);
    game.toggleMode();
    QCOMPARE(game.mode(), QStringLiteral("wrap"));
    QCOMPARE(game.modeLabel(), QStringLiteral("Wrap"));
    QCOMPARE(modes.count(), 1);
    game.setMode(QStringLiteral("sideways"));
    QCOMPARE(game.mode(), QStringLiteral("wrap"));
    QCOMPARE(modes.count(), 1);

    game.newGame(QStringLiteral("slow"));
    game.toggleMode();
    QCOMPARE(game.mode(), QStringLiteral("wrap"));
    game.backToStart();
    game.setMode(QStringLiteral("classic"));
    QCOMPARE(game.mode(), QStringLiteral("classic"));
}

void BridgeTests::highScoresOrderAndCap() {
    HighScores scores;
    const QDate day(2026, 8, 30);
    QCOMPARE(scores.insert(Mode::Classic, Difficulty::Slow, {100, 5, day}), 0);
    QCOMPARE(scores.insert(Mode::Classic, Difficulty::Slow, {300, 9, day}), 0);
    QCOMPARE(scores.insert(Mode::Classic, Difficulty::Slow, {200, 7, day}), 1);
    // A tie ranks below the older score.
    QCOMPARE(scores.insert(Mode::Classic, Difficulty::Slow, {200, 8, day}), 2);
    QCOMPARE(scores.entries(Mode::Classic, Difficulty::Slow).at(2).length, 8);
    QCOMPARE(scores.best(Mode::Classic, Difficulty::Slow), 300);
    QCOMPARE(scores.best(Mode::Wrap, Difficulty::Slow), 0);

    for (int i = 0; i < 20; ++i)
        scores.insert(Mode::Wrap, Difficulty::Fast, {i * 10, 4, day});
    QCOMPARE(int(scores.entries(Mode::Wrap, Difficulty::Fast).size()), HighScores::kMaxEntries);
    QCOMPARE(scores.entries(Mode::Wrap, Difficulty::Fast).front().score, 190);
    QCOMPARE(scores.entries(Mode::Wrap, Difficulty::Fast).back().score, 100);
    QCOMPARE(scores.insert(Mode::Wrap, Difficulty::Fast, {50, 4, day}), -1);
    QCOMPARE(scores.insert(Mode::Wrap, Difficulty::Fast, {150, 4, day}), 5);
    QCOMPARE(scores.entries(Mode::Wrap, Difficulty::Fast).back().score, 110);
}

void BridgeTests::highScoresAreSeparatePerModeAndDifficulty() {
    HighScores scores;
    const QDate day(2026, 8, 30);
    for (int m = 0; m < kModeCount; ++m) {
        for (int d = 0; d < kDifficultyCount; ++d)
            scores.insert(Mode(m), Difficulty(d), {100 * (m + 1) + d, 4, day});
    }
    for (int m = 0; m < kModeCount; ++m) {
        for (int d = 0; d < kDifficultyCount; ++d) {
            QCOMPARE(int(scores.entries(Mode(m), Difficulty(d)).size()), 1);
            QCOMPARE(scores.best(Mode(m), Difficulty(d)), 100 * (m + 1) + d);
        }
    }
    QCOMPARE(HighScores::idFor(Mode::Wrap, Difficulty::Fast), QStringLiteral("wrap-fast"));
}

void BridgeTests::highScoresRoundTripThroughSettings() {
    {
        HighScores scores;
        scores.insert(Mode::Classic, Difficulty::Normal, {4200, 30, QDate(2026, 8, 30)});
        scores.insert(Mode::Classic, Difficulty::Normal, {900, 12, QDate(2026, 8, 29)});
        scores.insert(Mode::Wrap, Difficulty::Fast, {7000, 44, QDate(2026, 8, 28)});
        scores.save();
    }
    HighScores loaded;
    loaded.load();
    QCOMPARE(int(loaded.entries(Mode::Classic, Difficulty::Normal).size()), 2);
    QCOMPARE(loaded.entries(Mode::Classic, Difficulty::Normal).front().score, 4200);
    QCOMPARE(loaded.entries(Mode::Classic, Difficulty::Normal).front().length, 30);
    QCOMPARE(loaded.entries(Mode::Classic, Difficulty::Normal).front().date, QDate(2026, 8, 30));
    QCOMPARE(loaded.entries(Mode::Classic, Difficulty::Normal).back().date, QDate(2026, 8, 29));
    QCOMPARE(loaded.best(Mode::Wrap, Difficulty::Fast), 7000);
    QVERIFY(loaded.entries(Mode::Wrap, Difficulty::Slow).empty());

    OmasnakeGame game;
    QCOMPARE(game.highScores().size(), 3);
    QCOMPARE(game.highScores().first().toMap().value(QStringLiteral("label")).toString(), QStringLiteral("Normal"));
    QCOMPARE(game.bests().value(QStringLiteral("classic-normal")).toInt(), 4200);
    QCOMPARE(game.bests().value(QStringLiteral("wrap-fast")).toInt(), 7000);
    QCOMPARE(game.best(), 4200);
}

void BridgeTests::lastModeAndDifficultyAreRemembered() {
    {
        OmasnakeGame game;
        game.setStepInterval(0);
        game.toggleMode();
        game.newGame(QStringLiteral("slow"));
        game.backToStart();
    }
    OmasnakeGame game;
    QCOMPARE(game.mode(), QStringLiteral("wrap"));
    QCOMPARE(game.difficulty(), QStringLiteral("slow"));
    QCOMPARE(game.difficultyLabel(), QStringLiteral("Slow"));
}

void BridgeTests::windowGeometryRoundTrips() {
    OmasnakeGame game;
    QVERIFY(!game.windowGeometry().value(QStringLiteral("valid")).toBool());
    game.saveWindowGeometry(-20, 30, 1000, 700, true);
    const QVariantMap geometry = game.windowGeometry();
    QVERIFY(geometry.value(QStringLiteral("valid")).toBool());
    QCOMPARE(geometry.value(QStringLiteral("x")).toInt(), -20);
    QCOMPARE(geometry.value(QStringLiteral("width")).toInt(), 1000);
    QVERIFY(geometry.value(QStringLiteral("maximized")).toBool());
}
