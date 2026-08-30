#include "scenario.h"

#include <QSettings>
#include <QTemporaryDir>

#include "solver.h"

namespace Scenario {

bool redirectSettings() {
    static QTemporaryDir dir;
    if (!dir.isValid())
        return false;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());
    return true;
}

void clearSettings() {
    QSettings settings;
    settings.clear();
    settings.sync();
}

OmasweeperGame *started(OmasweeperGame &game, Preset preset, quint32 seed) {
    game.setStepInterval(0);
    game.startGame(preset, seed);
    return &game;
}

void openFirst(OmasweeperGame &game) {
    game.revealAtCursor();
}

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

std::vector<int> hiddenCells(const Board &board, int wanted) {
    std::vector<int> cells;
    for (int i = 0; i < board.cellCount() && int(cells.size()) < wanted; ++i) {
        if (board.cell(i).state == CellState::Hidden)
            cells.push_back(i);
    }
    return cells;
}

}  // namespace Scenario
