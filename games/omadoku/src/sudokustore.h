#pragma once

#include <QJsonObject>
#include <QString>

// Everything Omadoku keeps between launches, and the one place its QSettings
// keys are written down (the best-time table and the window geometry carry
// their own, in OmaGames::ScoreTable and OmaGames::WindowGeometry).
//
// It stores and returns plain values and holds no state of its own; what to do
// with them — when to save, what a missing setting should default to — stays
// SudokuGame's business.
class SudokuStore {
public:
    struct SavedGame {
        QJsonObject board;  // empty when there is nothing to resume
        int elapsedSeconds = 0;
    };

    bool validateAsYouGo(bool fallback) const;
    void setValidateAsYouGo(bool validateAsYouGo) const;

    // Empty when nothing is stored, so the caller picks the default.
    QString clickMode() const;
    void setClickMode(const QString &clickMode) const;

    SavedGame savedGame() const;
    void saveGame(const QJsonObject &board, int elapsedSeconds) const;
    void clearSavedGame() const;
};
