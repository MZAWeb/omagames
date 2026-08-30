#pragma once

#include <QObject>
#include <QString>

// Ordering, the cap, ranks, persistence and reading the tables the games
// already wrote (common/src/scoretable.h).
class ScoreTableTests : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void init();

    void higherIsBetterRanksTheBiggestFirst();
    void lowerIsBetterRanksTheFastestFirst();
    void tiesRankBelowTheResultAlreadyThere();
    void theTableStopsAtItsCap();
    void aResultThatMissesTheCapIsNotKept();
    void tablesAreKeptPerCategory();
    void anUnknownCategoryIsIgnored();
    void bestIsTheTopOfTheTable();
    void extraFieldsSurviveARoundTrip();
    void everyCategoryRanksOnItsOwnField();
    void variantEntriesAreWhatQmlReads();
    void aStoredTableIsResortedAndTrimmed();
    void valuesBelowTheMinimumAreDropped();
    void savedTablesComeBackAfterARestart();

    void readsOmanixScores();
    void readsOmasnakeScores();
    void readsOmatrisScores();
    void readsOmadokuTimes();
    void readsOmasweeperTimes();

private:
    QString m_settingsDir;
};
