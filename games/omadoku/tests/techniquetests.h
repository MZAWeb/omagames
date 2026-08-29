#pragma once

#include <QObject>

// Each human technique on a grid crafted for it (games/omadoku/src/sudokutechniques.h).
class TechniqueTests : public QObject {
    Q_OBJECT
private slots:
    void firesOnItsOwnGridAndNothingEasierDoes_data();
    void firesOnItsOwnGridAndNothingEasierDoes();
    void idsRoundTrip();
};
