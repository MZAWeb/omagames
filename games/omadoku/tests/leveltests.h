#pragma once

#include <QObject>
#include <QString>

// The four levels: their ids, labels and the technique ladder each introduces
// (games/omadoku/src/sudokulevels.h), and how SudokuGame passes them to QML.
class LevelTests : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void init();

    void idsRoundTripAndSurviveNonsense();
    void everyLevelIntroducesRungsOfItsOwn();
    void theTimesTableRanksEachLevelFastestFirst();
    void difficultiesReachQmlWithLabels();

private:
    QString m_settingsDir;
};
