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
    void cursorMovesWithinTheGrid();
    void digitsGoIntoTheSelectedCellOnly();
    void clickModeDecidesWhatAKeypadClickDoes();
    void keyboardMappingIgnoresTheClickMode();
    void clickModeCyclesAndPersists();
    void highlightTogglesAndSwitchesDigits();
    void highlightWearsAFixedHighlighterYellow();
    void cursorValueFollowsTheCursor();
    void undoRestartAndEraseGoThroughTheBoard();
    void restartAsksBeforeWipingTheBoard();
    void restartIsNotExposedToQmlWithoutTheDialog();
    void untouchedPuzzleIsNotInProgress();
    void validateAsYouGoFlipsMidGame();
    void clockRunsOnlyWhilePlaying();

private:
    QString m_settingsDir;
};
