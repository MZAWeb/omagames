#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QVariant>

#include "cellmodel.h"
#include "sudoku.h"
#include "sudokugame.h"

// Reading a SudokuGame the way QML does — through the cell model and the
// selection list — so every suite pokes at the bridge's own surface.
namespace TestSupport {

inline int cellInt(SudokuGame *game, int cell, CellModel::Role role) {
    QAbstractListModel *model = game->cells();
    return model->data(model->index(cell, 0), role).toInt();
}

inline bool cellBool(SudokuGame *game, int cell, CellModel::Role role) {
    QAbstractListModel *model = game->cells();
    return model->data(model->index(cell, 0), role).toBool();
}

inline int valueOf(SudokuGame *game, int cell) {
    return cellInt(game, cell, CellModel::ValueRole);
}

inline int notesOf(SudokuGame *game, int cell) {
    return cellInt(game, cell, CellModel::NotesRole);
}

inline int firstGiven(SudokuGame *game) {
    for (int i = 0; i < Sudoku::kCells; ++i) {
        if (cellBool(game, i, CellModel::GivenRole))
            return i;
    }
    return -1;
}

inline QList<int> selectionOf(const SudokuGame &game) {
    QList<int> indices;
    for (const QVariant &index : game.selectedIndices())
        indices << index.toInt();
    return indices;
}

// A row of cells the player may write in, so a sweep across it is meaningful.
inline QList<int> emptyRow(SudokuGame *game) {
    for (int row = 0; row < Sudoku::kSize; ++row) {
        QList<int> empties;
        for (int column = 0; column < Sudoku::kSize; ++column) {
            const int index = row * Sudoku::kSize + column;
            if (valueOf(game, index) == 0)
                empties << index;
        }
        if (empties.size() >= 3)
            return empties;
    }
    return {};
}

}  // namespace TestSupport
