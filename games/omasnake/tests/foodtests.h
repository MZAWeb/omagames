#pragma once

#include <QObject>

// What the board puts out and what it pays: where a dot may land, the bonus
// every fifth food brings, and the forty dots a seed promises.
class FoodTests : public QObject {
    Q_OBJECT

private slots:
    void foodNeverLandsOnTheSnake();
    void fillingTheBoardEndsThePerfectGame();
    void bonusFollowsEveryFifthFood();
    void bonusExpiresAfterItsLifetime();
    void bonusEatenScoresItsFlatFifty();
    void sameSeedSameGame();
};
