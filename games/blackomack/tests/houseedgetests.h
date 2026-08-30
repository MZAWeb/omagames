#pragma once

#include <QObject>

// A whole shoe game played by the book has a known, small edge; this suite
// is the guard rail against a rule change that quietly moves it.
class HouseEdgeTests : public QObject {
    Q_OBJECT
private slots:
    void houseEdgeStaysInTheKnownBand();
};
