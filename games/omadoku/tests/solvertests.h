#pragma once

#include <QObject>

// Rules, solving and solution counting (games/omadoku/src/sudoku.h).
class SolverTests : public QObject {
    Q_OBJECT
private slots:
    void placementRejectsRowColumnAndBoxConflicts();
    void completeGridIsRecognised();
    void solvesAKnownPuzzle();
    void countsSolutionsUpToLimit();
    void reportsNoSolutionForContradictoryGrid();
    void wrongCellsIgnoresEmptyCells();
};
