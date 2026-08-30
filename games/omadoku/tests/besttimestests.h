#pragma once

#include <QObject>
#include <QString>

// How a win reaches the level's table and what QML is shown of it. The table
// itself is OmaGames::ScoreTable, tested in common/tests.
class BestTimesTests : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void init();

    void aWinIsRecordedWithItsRank();
    void aHandEditedZeroIsNotATime();
    void restartingOrLeavingRecordsNothing();

private:
    QString m_settingsDir;
};
