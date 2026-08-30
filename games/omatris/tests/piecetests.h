#pragma once

#include <QObject>

// The pieces themselves: the SRS spawn shapes, the rotation states, the kick
// tables, and the seven-bag they come out of.
class PieceTests : public QObject {
    Q_OBJECT

private slots:
    void spawnOrientationsMatchTheGuideline();
    void rotationStatesCycleBothWays();
    void kickTablesMatchTheGuideline();
    void iPieceKicksOffAWall();
    void tPieceKicksAgainstBothWalls();
    void oPieceNeverMovesWhenTurned();
    void spawnBoxTrimsAPieceToItsOwnCells();
    void sevenBagDealsEveryPieceOncePerSeven();
    void sameSeedPlaysTheSameGame();
};
