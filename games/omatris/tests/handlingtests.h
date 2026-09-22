#pragma once

#include <QObject>

// The player's handling on its own: the defaults, the range of each setting,
// the stored form and what the panel reads (games/omatris/src/handling.h).
class HandlingTests : public QObject {
    Q_OBJECT

private slots:
    void defaultsAreTheValuesOmatrisAlwaysPlayedWith();
    void delayAndRepeatStopAtTheEndsOfTheirRange();
    void softDropStepsThroughItsSpeedsToInstant();
    void storedFormRoundTripsAndRejectsNonsense();
    void rowsDescribeEachSetting();
};
