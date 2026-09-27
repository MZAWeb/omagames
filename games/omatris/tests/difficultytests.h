#pragma once

#include <QObject>

// Challenge's difficulty rating: what it measures, which way each feature
// pushes it, its range, and the game keeping it current as pieces settle.
class DifficultyTests : public QObject {
    Q_OBJECT

private slots:
    void measureReadsTheBoard();
    void everyFeaturePushesTheRatingUp();
    void ratingStaysBetweenOneAndAHundred();
    void freshDealsSpreadAcrossTheScale();
    void theGameRatesEverySettledBoard();
};
