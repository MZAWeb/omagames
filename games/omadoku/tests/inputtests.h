#pragma once

#include <QObject>
#include <QString>

// How a key press becomes a digit, and how a digit reaches the board
// (games/omadoku/src/sudokukeys.h, games/omadoku/src/sudokugame.h).
class InputTests : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void init();

    void plainDigitKeysResolve();
    void shiftedDigitsResolveOnUsLayouts();
    void modifiersAndOtherKeysResolveToNothing();
    void theBridgeAnswersTheSameAsTheHelper();
    void aShiftHeldSweepNeverWritesANote();

private:
    QString m_settingsDir;
};
