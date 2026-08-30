#include "playtests.h"

#include <QSignalSpy>
#include <QtTest>

#include "scenario.h"
#include "solver.h"

using namespace Scenario;

void PlayTests::initTestCase() {
    QVERIFY(Scenario::redirectSettings());
}

void PlayTests::init() {
    Scenario::clearSettings();
}

void PlayTests::cursorMovesAndStopsAtTheEdges() {
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

void PlayTests::firstRevealOpensASafeZeroRegion() {
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

void PlayTests::flagsCountDownAndMayGoNegative() {
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

void PlayTests::revealOnANumberChords() {
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

void PlayTests::chordThroughTheBridge() {
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

void PlayTests::theClockRunsFromTheFirstRevealToTheEnd() {
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

void PlayTests::aSolvedBoardIsWonAndTimed() {
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

void PlayTests::hittingAMineLoses() {
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
