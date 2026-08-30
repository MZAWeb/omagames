#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QString>

#include "sudoku.h"
#include "sudokuboard.h"
#include "sudokugenerator.h"

// Helpers shared by the suites that exercise SudokuGame.
namespace TestSupport {

// Every suite must run against a throwaway config, never ~/.config/Omacom.
inline void redirectSettings(const QString &dir) {
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir);
}

// Builds a board from a fixed seed with `blanks` cells left empty and stores it
// the way the game persists an in-progress puzzle.
inline SudokuBoard installSavedGame(int blanks, quint32 seed) {
    SudokuBoard board;
    board.setPuzzle(SudokuGenerator::generate(Difficulty::Easy, seed));
    const Sudoku::Grid solution = board.puzzle().solution;
    int left = blanks;
    for (int i = Sudoku::kCells - 1; i >= 0; --i) {
        if (board.isGiven(i))
            continue;
        if (left > 0) {
            --left;
            continue;
        }
        board.setValue(i, solution[size_t(i)]);
    }

    QJsonObject json = board.toJson();
    json.insert(QStringLiteral("elapsed"), 42);
    QSettings settings;
    settings.setValue(QStringLiteral("state/v1"),
                      QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Compact)));
    settings.sync();
    return board;
}

}  // namespace TestSupport
