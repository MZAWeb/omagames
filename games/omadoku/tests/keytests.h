#pragma once

#include <QObject>
#include <QString>

// How a key press becomes a digit, whatever the layout or the keycode under
// it (games/omadoku/src/sudokukeys.h).
class KeyTests : public QObject {
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
