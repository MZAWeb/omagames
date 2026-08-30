#pragma once

#include <QObject>

// The rules of the minefield, on hand-placed mines.
class BoardTests : public QObject {
    Q_OBJECT

private slots:
    void numbersCountAdjacentMines();
    void zeroCascadesOutwards();
    void firstRevealPlacesMinesAroundASafeZero();
    void revealingAMineLosesAndReportsWrongFlags();
    void flagsToggleAndTheCounterGoesNegative();
    void flaggedAndRevealedCellsIgnoreReveal();
    void chordRevealsAroundASatisfiedNumber();
    void chordWithAWrongFlagLoses();
    void chordNeedsMatchingFlags();
    void winningNeedsNoFlags();
    void nothingMovesAfterTheGameEnds();
    void mineCountIsClamped();
    void sameSeedSameMines();
    void jsonRoundTrip();
    void jsonRejectsGarbage();
};
