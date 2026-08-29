#pragma once

#include <QObject>
#include <QString>

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
    void exposesDifficultiesWithLabels();
    void padModeDispatchesClicks();
    void padModeCyclesAndPersists();
    void ctrlAndShiftPathsIgnoreThePadMode();
    void highlightTogglesAndSwitchesDigits();
    void padHighlightsWhenNothingIsSelected();
    void selectedValueFollowsTheSelection();
    void undoRestartAndEraseGoThroughTheBoard();
    void untouchedPuzzleIsNotInProgress();
    void clockRunsOnlyWhilePlaying();

private:
    QString m_settingsDir;
};
