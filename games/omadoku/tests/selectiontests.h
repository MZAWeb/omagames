#pragma once

#include <QObject>
#include <QString>

// Which cells the keyboard acts on: the cursor and the multi-cell selection
// (games/omadoku/src/sudokuselection.h), and how SudokuGame drives them.
class SelectionTests : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void init();

    void selectingCollapsesOntoOneCell();
    void movingClampsAtTheEdges();
    void extendingTakesEveryCellCrossedAlongOnce();
    void togglingAddsAndDropsCellsAndCarriesTheCursor();
    void collapsingLeavesTheCursorWhereItIs();

    void cursorMovesWithinTheGrid();
    void ctrlClickTogglesCellsInAndOutOfTheSelection();
    void shiftArrowsSweepTheCellsTheCursorCrosses();
    void plainMovesAndClicksCollapseTheSelection();
    void notesGoToEverySelectedEmptyCellAsOneStep();
    void aValueGoesToTheCursorAndFoldsTheSelection();
    void escapeUnwindsSelectionThenHighlight();

private:
    QString m_settingsDir;
};
