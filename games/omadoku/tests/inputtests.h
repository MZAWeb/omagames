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
    void shiftedDigitsResolveOnEveryLayout();
    void modifiersAndOtherKeysResolveToNothing();
    void theBridgeAnswersTheSameAsTheHelper();

private:
    QString m_settingsDir;
};
