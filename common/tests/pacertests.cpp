#include "pacertests.h"

#include <QtTest>

#include "pacer.h"

using OmaGames::Pacer;

void PacerTests::intervalOnlySignalsAChange() {
    Pacer pacer(Pacer::Repeating, []() {});
    QCOMPARE(pacer.interval(), 0);
    QVERIFY(pacer.setInterval(16));
    QCOMPARE(pacer.interval(), 16);
    QVERIFY(!pacer.setInterval(16));
}

void PacerTests::repeatingRunsWhileTheBridgeSaysSo() {
    Pacer pacer(Pacer::Repeating, []() {});
    pacer.setInterval(1000);
    QVERIFY(!pacer.isActive());  // the interval alone schedules nothing
    pacer.setRunning(true);
    QVERIFY(pacer.isActive());
    pacer.setRunning(false);
    QVERIFY(!pacer.isActive());
}

void PacerTests::aZeroIntervalNeverSchedulesARepeatingStep() {
    int steps = 0;
    Pacer pacer(Pacer::Repeating, [&steps]() { ++steps; });
    pacer.setInterval(0);
    pacer.setRunning(true);
    QVERIFY(!pacer.isActive());
    QCOMPARE(steps, 0);  // a headless test drives step() itself
}

void PacerTests::repeatingFiresOnTheInterval() {
    int steps = 0;
    Pacer pacer(Pacer::Repeating, [&steps]() { ++steps; });
    pacer.setInterval(1);
    pacer.setRunning(true);
    QTRY_VERIFY(steps >= 2);
    QVERIFY(pacer.isActive());
    pacer.stop();
}

void PacerTests::singleShotFiresOnce() {
    int steps = 0;
    Pacer pacer(Pacer::SingleShot, [&steps]() { ++steps; });
    pacer.runIn(1);
    QTRY_COMPARE(steps, 1);
    QVERIFY(!pacer.isActive());
    QTest::qWait(10);
    QCOMPARE(steps, 1);
}

void PacerTests::aZeroDelayRunsTheStepSynchronously() {
    int steps = 0;
    Pacer pacer(Pacer::SingleShot, [&steps]() { ++steps; });
    pacer.runIn(0);
    QCOMPARE(steps, 1);  // already run, not queued
    QVERIFY(!pacer.isActive());
}

void PacerTests::stopCancelsAPendingStep() {
    int steps = 0;
    Pacer pacer(Pacer::SingleShot, [&steps]() { ++steps; });
    pacer.runIn(50);
    QVERIFY(pacer.isActive());
    pacer.stop();
    QTest::qWait(80);
    QCOMPARE(steps, 0);
}
