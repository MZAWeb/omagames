#pragma once

#include <QObject>

#include "sudokuboard.h"

// Playable board state: entry rules, notes, validation, undo, persistence
// (games/omadoku/src/sudokuboard.h).
class BoardTests : public QObject {
    Q_OBJECT
private slots:
    void init();

    void givensAreImmutable();
    void entryClearsOwnAndPeerNotes();
    void notesToggleOnlyInEmptyCells();
    void eraseClearsValueAndNotes();
    void undoRestoresValuesAndNotes();
    void undoKeepsAtLeastAHundredLevels();
    void checkAsYouGoFlagsWrongEntries();
    void checkWhenFullDefersFlagging();
    void solvedWhenEveryCellMatches();
    void restartClearsEntriesAndHistory();
    void countsFilledCellsAndDigits();
    void countsOnlyPlayerEntries();
    void jsonRoundTripsBoardState();

private:
    int emptyCell(int nth = 0) const;
    int givenCell() const;
    int emptyPeerOf(int index) const;
    int emptyNonPeerOf(int index) const;
    void fillEverythingCorrectlyExcept(int index);

    SudokuBoard m_board;
};
