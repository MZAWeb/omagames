#pragma once

#include <QObject>

// What Omatris keeps: a played run's place in its mode's table, the tables
// written before the move to ScoreTable, the last mode chosen, the ghost and
// the window.
class PersistenceTests : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void scriptedMarathonTopsOutAndRecordsAScore();
    void scriptedSprintRecordsItsTime();
    void savedHighScoresReachQml();
    void lastModeIsRemembered();
    void ghostToggleIsRemembered();
    void windowGeometryRoundTrips();
};
