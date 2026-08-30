#include "autoshifttests.h"

#include <QtTest>

#include "autoshift.h"

namespace {

// Ticks the clock `count` times and reports how many cells it asked for, and
// on which tick the first of them fell due.
int shifts(AutoShift &shift, int count, int *firstAt = nullptr) {
    int moves = 0;
    for (int i = 1; i <= count; ++i) {
        if (shift.tick() == 0)
            continue;
        if (firstAt && moves == 0)
            *firstAt = i;
        ++moves;
    }
    return moves;
}

}  // namespace

void AutoShiftTests::nothingRepeatsWhileNoKeyIsHeld() {
    AutoShift shift;
    QCOMPARE(shift.direction(), 0);
    QCOMPARE(shifts(shift, 10 * AutoShift::kDelayTicks), 0);
}

void AutoShiftTests::theFirstRepeatWaitsOutTheDelay() {
    AutoShift shift;
    shift.press(-1);
    QCOMPARE(shift.direction(), -1);
    // The press itself is the caller's move: nothing repeats before the wait.
    QCOMPARE(shifts(shift, AutoShift::kDelayTicks - 1), 0);
    QCOMPARE(shift.tick(), -1);
}

void AutoShiftTests::furtherRepeatsComeAtTheRepeatRate() {
    AutoShift shift;
    shift.press(1);
    int firstAt = 0;
    const int ticks = AutoShift::kDelayTicks + 6 * AutoShift::kRepeatTicks;
    QCOMPARE(shifts(shift, ticks, &firstAt), 7);
    QCOMPARE(firstAt, AutoShift::kDelayTicks);
    // Every tick between two repeats asks for nothing.
    for (int i = 1; i < AutoShift::kRepeatTicks; ++i)
        QCOMPARE(shift.tick(), 0);
    QCOMPARE(shift.tick(), 1);
}

void AutoShiftTests::theOtherDirectionTakesOverAndWaitsAgain() {
    AutoShift shift;
    shift.press(-1);
    QCOMPARE(shifts(shift, AutoShift::kDelayTicks), 1);
    shift.press(1);
    QCOMPARE(shift.direction(), 1);
    // The new key starts from scratch, delay and all.
    QCOMPARE(shifts(shift, AutoShift::kDelayTicks - 1), 0);
    QCOMPARE(shift.tick(), 1);
}

void AutoShiftTests::lettingGoStopsTheRepeats() {
    AutoShift shift;
    shift.press(-1);
    QCOMPARE(shifts(shift, AutoShift::kDelayTicks), 1);
    shift.release(-1);
    QCOMPARE(shift.direction(), 0);
    QCOMPARE(shifts(shift, 4 * AutoShift::kDelayTicks), 0);
    // Pressing it again waits the whole delay out once more.
    shift.press(-1);
    int firstAt = 0;
    QCOMPARE(shifts(shift, AutoShift::kDelayTicks, &firstAt), 1);
    QCOMPARE(firstAt, AutoShift::kDelayTicks);
}

void AutoShiftTests::theKeyStillDownKeepsShifting() {
    AutoShift shift;
    shift.press(-1);
    shift.press(1);
    // Rolling from one key to the other: the first one coming up changes
    // nothing, because the second one is the one holding the shift.
    shift.release(-1);
    QCOMPARE(shift.direction(), 1);
    QCOMPARE(shifts(shift, AutoShift::kDelayTicks), 1);
    shift.release(1);
    QCOMPARE(shift.direction(), 0);
    QCOMPARE(shifts(shift, 4 * AutoShift::kDelayTicks), 0);
}

void AutoShiftTests::clearingForgetsTheHeldKey() {
    AutoShift shift;
    shift.press(1);
    QCOMPARE(shifts(shift, AutoShift::kDelayTicks - 1), 0);
    shift.clear();
    QCOMPARE(shift.direction(), 0);
    QCOMPARE(shifts(shift, 4 * AutoShift::kDelayTicks), 0);
    // And the wait it had partly served is gone with it.
    shift.press(1);
    int firstAt = 0;
    QCOMPARE(shifts(shift, AutoShift::kDelayTicks, &firstAt), 1);
    QCOMPARE(firstAt, AutoShift::kDelayTicks);
}
