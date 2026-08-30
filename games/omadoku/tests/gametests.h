#pragma once

#include <QObject>
#include <QString>

// The QML bridge itself: screen flow, entering and undoing digits, the
// restart question and the clock (games/omadoku/src/sudokugame.h).
class GameTests : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void init();

    void startsOnTheStartScreen();
    void newGameSelectsTheFirstEmptyCell();
    void digitsGoIntoTheSelectedCellOnly();
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
