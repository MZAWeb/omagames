#pragma once

#include <QObject>

// Each technique on a crafted position, then the solver against the truth
// over many seeded boards.
class SolverTests : public QObject {
    Q_OBJECT

private slots:
    void singlesFindMines();
    void singlesFindSafeCells();
    void subsetsSplitOverlappingNumbers();
    void enumerationSolvesWhatSubsetsCannot();
    void enumerationUsesTheMineCountBehindTheFrontier();
    void enumerationFindsMinesBehindTheFrontier();
    void exhaustedBudgetDeducesNothing();
    void solveStopsAtAFiftyFifty();
    void solveNeverMarksAWrongCell();
    void solvabilityFromAFirstClick();
    void deduceIsSilentWhenThereIsNothingToSee();
};
