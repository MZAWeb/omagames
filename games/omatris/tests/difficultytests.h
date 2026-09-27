#pragma once

#include <QObject>

// Challenge's difficulty rating: what it measures, that it follows the work
// left rather than the board's looks, its range, and the game keeping it
// current as pieces settle.
class DifficultyTests : public QObject {
    Q_OBJECT

private slots:
    void measureFindsTheRowsThatHaveToGo();
    void buildingBesideTheGapsCostsNothing();
    void fillingAGapHelpsAndBuryingOneHurts();
    void ratingStaysBetweenOneAndAHundred();
    void freshDealsSpreadAcrossTheScale();
    void theGameRatesEverySettledBoard();
};
