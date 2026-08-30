#pragma once

#include <QObject>

// Where the seats sit on the felt and the order play travels round it
// (games/blackomack/src/seatlayout.h).
class SeatLayoutTests : public QObject {
    Q_OBJECT
private slots:
    void seatSlotsNeverOverlap();
    void seatSlotsSweepAroundTheArc();
    void playOrderMatchesTheSeatingArc();
};
