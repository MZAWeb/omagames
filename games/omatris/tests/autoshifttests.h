#pragma once

#include <QObject>

// Delayed auto shift on its own: the wait, the repeat rate, and what a second
// key does to the first (games/omatris/src/autoshift.h).
class AutoShiftTests : public QObject {
    Q_OBJECT

private slots:
    void nothingRepeatsWhileNoKeyIsHeld();
    void theFirstRepeatWaitsOutTheDelay();
    void furtherRepeatsComeAtTheRepeatRate();
    void theOtherDirectionTakesOverAndWaitsAgain();
    void lettingGoStopsTheRepeats();
    void theKeyStillDownKeepsShifting();
    void clearingForgetsTheHeldKey();
};
