#pragma once

#include <QObject>

// The interval property and the two ways a bridge asks for a step
// (common/src/pacer.h).
class PacerTests : public QObject {
    Q_OBJECT
private slots:
    void intervalOnlySignalsAChange();
    void repeatingRunsWhileTheBridgeSaysSo();
    void aZeroIntervalNeverSchedulesARepeatingStep();
    void repeatingFiresOnTheInterval();
    void singleShotFiresOnce();
    void aZeroDelayRunsTheStepSynchronously();
    void stopCancelsAPendingStep();
};
