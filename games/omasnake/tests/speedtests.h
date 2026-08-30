#pragma once

#include <QObject>

// The speed ladder: how fast the snake starts, how much every few foods take
// off, and how the three difficulties sit against each other.
class SpeedTests : public QObject {
    Q_OBJECT

private slots:
    void speedRampsWithFoodEaten();
    void theThreeDifficultiesFormALadder();
};
