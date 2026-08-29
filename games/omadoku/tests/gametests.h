#pragma once

#include <QObject>

#include "sudokuboard.h"

// The QML bridge: screen flow, selection, digit entry, undo and persistence
// (games/omadoku/src/sudokugame.h).
class GameTests : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void init();

    void startsOnTheStartScreen();
    void newGameSelectsTheFirstEmptyCell();
    void selectionMovesWithinTheGrid();
    void digitsGoIntoTheSelectedCellOnly();
    void notesModeWritesPencilMarks();
    void selectedValueFollowsTheSelection();
    void undoRestartAndEraseGoThroughTheBoard();
    void winningSwitchesStateAndClearsTheSave();
    void checkAsYouGoIsRemembered();
    void savedGameSurvivesRestart();
    void solvedSaveIsNotOffered();
    void clockRunsOnlyWhilePlaying();

private:
    // Builds a board from a fixed seed with `blanks` cells left empty and
    // stores it the way the game persists an in-progress puzzle.
    SudokuBoard installSavedGame(int blanks);
    int firstEmptyCell(const SudokuBoard &board) const;

    QString m_settingsDir;
};
