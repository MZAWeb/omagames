#pragma once

#include <QObject>
#include <QString>

// The five fastest solves per level, and how a win reaches them
// (games/omadoku/src/besttimes.h).
class BestTimesTests : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void init();

    void fastestFirstAndOnlyFiveKept();
    void tiesRankBelowTheTimeAlreadyThere();
    void tablesAreKeptPerLevel();
    void tablesSurviveARestart();
    void aWinIsRecordedWithItsRank();
    void restartingOrLeavingRecordsNothing();

private:
    QString m_settingsDir;
};
