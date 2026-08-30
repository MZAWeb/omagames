#include "bridgetests.h"

#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include "omasweepergame.h"
#include "solver.h"

namespace {

constexpr quint32 kSeed = 20260830u;

const auto kStart = QStringLiteral("start");
const auto kPlaying = QStringLiteral("playing");
const auto kWon = QStringLiteral("won");
const auto kLost = QStringLiteral("lost");

OmasweeperGame *started(OmasweeperGame &game, Preset preset, quint32 seed = kSeed) {
    game.setStepInterval(0);
    game.startGame(preset, seed);
    return &game;
}

// Opens the middle of the board, which is what generates it.
void openFirst(OmasweeperGame &game) {
    game.revealAtCursor();
}

// Plays the board with the logical solver, but only ever through the
// invokables a player would use, until it is won or logic runs out.
void solveThroughBridge(OmasweeperGame &game) {
    while (game.phase() == kPlaying) {
        const Deduction step = Solver::deduce(*game.board());
        if (step.empty())
            return;
        for (int index : step.mines) {
            const QPoint p = game.board()->point(index);
            game.toggleFlag(p.x(), p.y());
        }
        for (int index : step.safe) {
            const QPoint p = game.board()->point(index);
            game.reveal(p.x(), p.y());
        }
    }
}

int firstMine(const Board &board) {
    const std::vector<int> mines = board.mines();
    return mines.empty() ? -1 : mines.front();
}

// A hidden cell whose flag can be toggled without touching the opening.
std::vector<int> hiddenCells(const Board &board, int wanted) {
    std::vector<int> cells;
    for (int i = 0; i < board.cellCount() && int(cells.size()) < wanted; ++i) {
        if (board.cell(i).state == CellState::Hidden)
            cells.push_back(i);
    }
    return cells;
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

void BridgeTests::startsOnTheStartScreenWithPresets() {
    OmasweeperGame game;
    QCOMPARE(game.phase(), kStart);
    QCOMPARE(game.status(), QStringLiteral("ready"));
    QVERIFY(game.board() == nullptr);

    const QVariantList presets = OmasweeperGame::presets();
    QCOMPARE(presets.size(), 3);
    QCOMPARE(presets.at(0).toMap().value(QStringLiteral("id")).toString(), QStringLiteral("beginner"));
    QCOMPARE(presets.at(2).toMap().value(QStringLiteral("label")).toString(), QStringLiteral("Expert"));
    QCOMPARE(presets.at(2).toMap().value(QStringLiteral("width")).toInt(), 30);
    QCOMPARE(presets.at(2).toMap().value(QStringLiteral("mines")).toInt(), 99);
    // Never played: the counter still shows the preset's mines.
    QCOMPARE(game.remainingMines(), 10);
}

void BridgeTests::newGameExposesThePreset() {
    OmasweeperGame game;
    game.setStepInterval(0);
    game.newGame(QStringLiteral("intermediate"));
    QCOMPARE(game.preset(), QStringLiteral("intermediate"));
    QCOMPARE(game.presetLabel(), QStringLiteral("Intermediate"));
    QCOMPARE(game.width(), 16);
    QCOMPARE(game.height(), 16);
    QCOMPARE(game.mines(), 40);
    QCOMPARE(game.phase(), kPlaying);
    QCOMPARE(game.status(), QStringLiteral("ready"));
    QCOMPARE(game.remainingMines(), 40);
    QCOMPARE(game.flags(), 0);
    QCOMPARE(game.elapsedSeconds(), 0);
    QVERIFY(game.noGuess());
    // The cursor starts in the middle, and no mines exist until a reveal.
    QCOMPARE(game.cursorX(), 8);
    QCOMPARE(game.cursorY(), 8);
    QVERIFY(!game.board()->minesPlaced());
}

void BridgeTests::cursorMovesAndStopsAtTheEdges() {
    OmasweeperGame game;
    started(game, Preset::Beginner);
    QSignalSpy spy(&game, &OmasweeperGame::cursorChanged);
    game.moveCursor(1, -1);
    QCOMPARE(game.cursorX(), 5);
    QCOMPARE(game.cursorY(), 3);
    QCOMPARE(spy.count(), 1);

    for (int i = 0; i < 20; ++i)
        game.moveCursor(-1, -1);
    QCOMPARE(game.cursorX(), 0);
    QCOMPARE(game.cursorY(), 0);
    const int stopped = spy.count();
    // Pushing past the edge is not a change, so nothing is notified.
    game.moveCursor(-1, 0);
    QCOMPARE(spy.count(), stopped);

    for (int i = 0; i < 20; ++i)
        game.moveCursor(1, 1);
    QCOMPARE(game.cursorX(), 8);
    QCOMPARE(game.cursorY(), 8);
}

void BridgeTests::firstRevealOpensASafeZeroRegion() {
    OmasweeperGame game;
    started(game, Preset::Beginner);
    QSignalSpy ripples(&game, &OmasweeperGame::revealRippled);
    openFirst(game);

    QCOMPARE(game.status(), kPlaying);
    QVERIFY(game.noGuess());
    QVERIFY(game.board()->minesPlaced());
    const QPoint opened(game.cursorX(), game.cursorY());
    QVERIFY(!game.board()->cell(opened).mine);
    QCOMPARE(game.board()->cell(opened).adjacent, quint8(0));
    // A zero opens at least its own 3x3.
    QVERIFY(game.board()->revealedCount() >= 9);
    QCOMPARE(ripples.count(), 1);

    const Ripple &ripple = game.lastReveal();
    QCOMPARE(ripple.cells.size(), game.board()->revealedCount());
    QCOMPARE(ripple.cells.size(), ripple.distances.size());
    QCOMPARE(ripple.cells.first(), game.board()->index(opened));
    QCOMPARE(ripple.distances.first(), 0);
    QVERIFY(ripple.maxDistance >= 1);
    // Breadth-first: distances never decrease along the cascade.
    for (int i = 1; i < ripple.distances.size(); ++i)
        QVERIFY(ripple.distances.at(i) >= ripple.distances.at(i - 1));
}

void BridgeTests::flagsCountDownAndMayGoNegative() {
    OmasweeperGame game;
    started(game, Preset::Beginner);
    const std::vector<int> cells = hiddenCells(*game.board(), 11);
    QCOMPARE(int(cells.size()), 11);
    for (int index : cells) {
        const QPoint p = game.board()->point(index);
        game.toggleFlag(p.x(), p.y());
    }
    QCOMPARE(game.flags(), 11);
    QCOMPARE(game.remainingMines(), -1);
    // The cursor followed the last flag.
    QCOMPARE(game.cursorX(), game.board()->point(cells.back()).x());

    const QPoint first = game.board()->point(cells.front());
    game.toggleFlag(first.x(), first.y());
    QCOMPARE(game.flags(), 10);
    QCOMPARE(game.remainingMines(), 0);
    // A flagged cell does not open, and does not draw the board either.
    const QPoint second = game.board()->point(cells.at(1));
    game.reveal(second.x(), second.y());
    QCOMPARE(game.board()->cell(second).state, CellState::Flagged);
    QCOMPARE(game.status(), QStringLiteral("ready"));
    QVERIFY(!game.board()->minesPlaced());

    // Flags planted before the first click survive the board being drawn.
    game.reveal(first.x(), first.y());
    QVERIFY(game.board()->minesPlaced());
    QCOMPARE(game.board()->cell(second).state, CellState::Flagged);
    QCOMPARE(game.flags(), 10);
}

void BridgeTests::revealOnANumberChords() {
    OmasweeperGame game;
    started(game, Preset::Beginner);
    openFirst(game);
    // A number on the edge of the opening, with its mines flagged, opens the
    // rest of its neighbours when it is "revealed" a second time.
    const Deduction step = Solver::deduce(*game.board());
    QVERIFY(!step.mines.empty());
    for (int index : step.mines) {
        const QPoint p = game.board()->point(index);
        game.toggleFlag(p.x(), p.y());
    }
    const QPoint mine = game.board()->point(step.mines.front());
    int number = -1;
    for (int n : game.board()->neighbours(game.board()->index(mine))) {
        const Cell &cell = game.board()->cell(n);
        if (cell.state != CellState::Revealed || cell.adjacent == 0)
            continue;
        int flags = 0;
        int hidden = 0;
        for (int m : game.board()->neighbours(n)) {
            flags += game.board()->cell(m).state == CellState::Flagged ? 1 : 0;
            hidden += game.board()->cell(m).state == CellState::Hidden ? 1 : 0;
        }
        if (flags == cell.adjacent && hidden > 0)
            number = n;
    }
    QVERIFY(number >= 0);

    const int before = game.board()->revealedCount();
    const QPoint p = game.board()->point(number);
    game.reveal(p.x(), p.y());
    QVERIFY(game.board()->revealedCount() > before);
    QVERIFY(game.status() != kLost);
}

void BridgeTests::chordThroughTheBridge() {
    OmasweeperGame game;
    started(game, Preset::Beginner);
    openFirst(game);
    // An unsatisfied number does nothing; the same number does once its
    // mines are flagged.
    const Deduction step = Solver::deduce(*game.board());
    QVERIFY(!step.mines.empty());
    const int mine = step.mines.front();
    int number = -1;
    for (int n : game.board()->neighbours(mine)) {
        if (game.board()->cell(n).state == CellState::Revealed && game.board()->cell(n).adjacent > 0)
            number = n;
    }
    QVERIFY(number >= 0);
    const QPoint numberPoint = game.board()->point(number);
    const int before = game.board()->revealedCount();
    game.chord(numberPoint.x(), numberPoint.y());
    QCOMPARE(game.board()->revealedCount(), before);

    const QPoint minePoint = game.board()->point(mine);
    game.toggleFlag(minePoint.x(), minePoint.y());
    for (int n : game.board()->neighbours(number)) {
        if (game.board()->cell(n).state == CellState::Hidden && game.board()->cell(n).mine) {
            const QPoint p = game.board()->point(n);
            game.toggleFlag(p.x(), p.y());
        }
    }
    // The cursor is sitting on a hidden flag, where a chord means nothing.
    game.chordAtCursor();
    QCOMPARE(game.board()->revealedCount(), before);
    game.chord(numberPoint.x(), numberPoint.y());
    QVERIFY(game.board()->revealedCount() > before);
    QVERIFY(game.status() != kLost);
}

void BridgeTests::theClockRunsFromTheFirstRevealToTheEnd() {
    OmasweeperGame game;
    started(game, Preset::Beginner);
    // Nothing has been opened, so nothing is timed.
    game.step();
    QCOMPARE(game.elapsedSeconds(), 0);

    openFirst(game);
    QSignalSpy spy(&game, &OmasweeperGame::elapsedSecondsChanged);
    for (int i = 0; i < 3; ++i)
        game.step();
    QCOMPARE(game.elapsedSeconds(), 3);
    QCOMPARE(spy.count(), 3);

    const QPoint mine = game.board()->point(firstMine(*game.board()));
    game.reveal(mine.x(), mine.y());
    QCOMPARE(game.phase(), kLost);
    game.step();
    QCOMPARE(game.elapsedSeconds(), 3);
}

void BridgeTests::aSolvedBoardIsWonAndTimed() {
    OmasweeperGame game;
    started(game, Preset::Beginner);
    openFirst(game);
    for (int i = 0; i < 42; ++i)
        game.step();
    solveThroughBridge(game);

    QCOMPARE(game.phase(), kWon);
    QCOMPARE(game.status(), kWon);
    QCOMPARE(game.board()->revealedCount(), game.board()->safeCount());
    QCOMPARE(game.elapsedSeconds(), 42);
    QCOMPARE(game.newBestRank(), 0);
    QCOMPARE(game.bests().value(QStringLiteral("beginner")).toInt(), 42);
    // The clock is stopped for good.
    game.step();
    QCOMPARE(game.elapsedSeconds(), 42);

    const QVariantList times = game.bestTimes();
    QCOMPARE(times.size(), 1);
    QCOMPARE(times.first().toMap().value(QStringLiteral("preset")).toString(), QStringLiteral("beginner"));
    QCOMPARE(times.first().toMap().value(QStringLiteral("seconds")).toInt(), 42);
    QCOMPARE(times.first().toMap().value(QStringLiteral("date")).toString(),
             QDate::currentDate().toString(Qt::ISODate));

    // A fresh bridge reads the same table back out of QSettings.
    OmasweeperGame reopened;
    QCOMPARE(reopened.bests().value(QStringLiteral("beginner")).toInt(), 42);
}

void BridgeTests::hittingAMineLoses() {
    OmasweeperGame game;
    started(game, Preset::Beginner);
    openFirst(game);
    // A wrong flag stands out once the board is turned over.
    const std::vector<int> safe = [&game] {
        std::vector<int> result;
        for (int i = 0; i < game.board()->cellCount(); ++i) {
            if (game.board()->cell(i).state == CellState::Hidden && !game.board()->cell(i).mine)
                result.push_back(i);
        }
        return result;
    }();
    QVERIFY(!safe.empty());
    const QPoint wrong = game.board()->point(safe.front());
    game.toggleFlag(wrong.x(), wrong.y());

    const int mine = firstMine(*game.board());
    const QPoint p = game.board()->point(mine);
    QSignalSpy phases(&game, &OmasweeperGame::phaseChanged);
    game.reveal(p.x(), p.y());

    QCOMPARE(game.phase(), kLost);
    QCOMPARE(game.explodedCell(), mine);
    QCOMPARE(game.cursorX(), p.x());
    QVERIFY(phases.count() >= 1);
    QCOMPARE(game.board()->cell(wrong).state, CellState::Flagged);
    QVERIFY(!game.board()->cell(wrong).mine);
    // Nothing else moves after a loss.
    const int revealed = game.board()->revealedCount();
    game.revealAtCursor();
    game.flagAtCursor();
    QCOMPARE(game.board()->revealedCount(), revealed);
    QCOMPARE(game.newBestRank(), -1);
}

void BridgeTests::generatorGivingUpClearsTheNoGuessFlag() {
    OmasweeperGame game;
    game.setStepInterval(0);
    // With a single attempt allowed the generator soon meets a layout it
    // cannot prove, and says so instead of pretending.
    game.setMaxAttemptsForTests(1);
    bool gaveUp = false;
    for (quint32 seed = 1; seed <= 20 && !gaveUp; ++seed) {
        game.startGame(Preset::Expert, seed);
        QVERIFY(game.noGuess());
        openFirst(game);
        gaveUp = !game.noGuess();
    }
    QVERIFY(gaveUp);
    // The board is still playable, it just carries no promise.
    QCOMPARE(game.phase(), kPlaying);
    QVERIFY(game.board()->revealedCount() >= 9);
}

void BridgeTests::restartKeepsThePresetAndDrawsANewBoard() {
    OmasweeperGame game;
    started(game, Preset::Intermediate);
    openFirst(game);
    for (int i = 0; i < 5; ++i)
        game.step();
    const std::vector<int> mines = game.board()->mines();

    game.restart();
    QCOMPARE(game.preset(), QStringLiteral("intermediate"));
    QCOMPARE(game.phase(), kPlaying);
    QCOMPARE(game.status(), QStringLiteral("ready"));
    QCOMPARE(game.elapsedSeconds(), 0);
    QCOMPARE(game.flags(), 0);
    QCOMPARE(game.board()->revealedCount(), 0);
    QVERIFY(!game.board()->minesPlaced());
    openFirst(game);
    QVERIFY(game.board()->mines() != mines);
}

void BridgeTests::backToStartClearsTheBoard() {
    OmasweeperGame game;
    started(game, Preset::Beginner);
    openFirst(game);
    game.backToStart();
    QCOMPARE(game.phase(), kStart);
    QVERIFY(game.board() == nullptr);
    // Actions on no board are harmless.
    game.revealAtCursor();
    game.flagAtCursor();
    game.chordAtCursor();
    game.moveCursor(1, 1);
    game.step();
    QCOMPARE(game.phase(), kStart);
    QCOMPARE(game.elapsedSeconds(), 0);
}

// Ordering, the cap and the ranks are ScoreTable's own tests now. What is
// Omasweeper's is that the tables are keyed by preset and that the JSON it
// wrote before the move still reads back: the string below is the literal
// output of the old BestTimes::toJson().
void BridgeTests::savedBestTimesReachQml() {
    QSettings settings;
    settings.setValue(
        QStringLiteral("scores/v1"),
        QStringLiteral(R"({"beginner":[{"date":"2026-08-27","seconds":22},)"
                       R"({"date":"2026-08-30","seconds":41}],)"
                       R"("expert":[{"date":"2026-03-09","seconds":602}],"intermediate":[]})"));
    settings.sync();

    OmasweeperGame game;
    const QVariantList rows = game.bestTimes();
    QCOMPARE(rows.size(), 3);
    const QVariantMap first = rows.first().toMap();
    QCOMPARE(first.value(QStringLiteral("preset")).toString(), QStringLiteral("beginner"));
    QCOMPARE(first.value(QStringLiteral("label")).toString(), QStringLiteral("Beginner"));
    QCOMPARE(first.value(QStringLiteral("seconds")).toInt(), 22);
    QCOMPARE(first.value(QStringLiteral("date")).toString(), QStringLiteral("2026-08-27"));
    QCOMPARE(rows.at(1).toMap().value(QStringLiteral("seconds")).toInt(), 41);
    QCOMPARE(rows.at(2).toMap().value(QStringLiteral("preset")).toString(), QStringLiteral("expert"));

    QCOMPARE(game.bests().value(QStringLiteral("beginner")).toInt(), 22);
    QCOMPARE(game.bests().value(QStringLiteral("expert")).toInt(), 602);
    QCOMPARE(game.bests().value(QStringLiteral("intermediate")).toInt(), 0);
}

void BridgeTests::lastPresetIsRemembered() {
    {
        OmasweeperGame game;
        game.setStepInterval(0);
        game.newGame(QStringLiteral("expert"));
        QCOMPARE(game.preset(), QStringLiteral("expert"));
    }
    OmasweeperGame reopened;
    QCOMPARE(reopened.preset(), QStringLiteral("expert"));
    QCOMPARE(reopened.presetLabel(), QStringLiteral("Expert"));
    // An unknown id keeps the remembered preset.
    reopened.setStepInterval(0);
    reopened.newGame(QStringLiteral("nonsense"));
    QCOMPARE(reopened.preset(), QStringLiteral("expert"));
}

void BridgeTests::windowGeometryRoundTrips() {
    OmasweeperGame game;
    QVERIFY(!game.windowGeometry().value(QStringLiteral("valid")).toBool());
    game.saveWindowGeometry(40, 50, 900, 700, true);
    const QVariantMap geometry = game.windowGeometry();
    QVERIFY(geometry.value(QStringLiteral("valid")).toBool());
    QCOMPARE(geometry.value(QStringLiteral("x")).toInt(), 40);
    QCOMPARE(geometry.value(QStringLiteral("width")).toInt(), 900);
    QVERIFY(geometry.value(QStringLiteral("maximized")).toBool());
}
