#pragma once

#include <QObject>
#include <QString>

// Everything SudokuGame keeps across launches: the setting, the in-progress
// puzzle and the win that clears it.
class PersistenceTests : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void init();

    void winningSwitchesStateAndClearsTheSave();
    void checkAsYouGoIsRemembered();
    void savedGameSurvivesRestart();
    void solvedSaveIsNotOffered();

private:
    QString m_settingsDir;
};
