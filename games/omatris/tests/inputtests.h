#pragma once

#include <QObject>

// The bridge under the keys, driven with stepInterval 0: the start screen, a
// new game, auto shift, rotation, hold, the drops, pause, and the shapes the
// hold and next boxes draw.
class InputTests : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void startsOnTheStartScreenWithModes();
    void newGameExposesEngineState();
    void autoShiftWaitsThenRepeats();
    void rotationHoldAndDropsGoThroughTheBridge();
    void pauseFreezesEverything();
    void pieceShapesFeedTheBoxes();
};
