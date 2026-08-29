#include "cellmodel.h"

#include "sudoku.h"
#include "sudokuboard.h"

CellModel::CellModel(const SudokuBoard *board, QObject *parent)
    : QAbstractListModel(parent), m_board(board) {}

int CellModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : Sudoku::kCells;
}

QVariant CellModel::data(const QModelIndex &index, int role) const {
    const int cell = index.row();
    if (cell < 0 || cell >= Sudoku::kCells)
        return {};
    switch (role) {
    case GivenRole:
        return m_board->isGiven(cell);
    case ValueRole:
        return m_board->value(cell);
    case NotesRole:
        return int(m_board->notes(cell));
    case WrongRole:
        return m_board->isWrong(cell);
    default:
        return {};
    }
}

QHash<int, QByteArray> CellModel::roleNames() const {
    return {
        {GivenRole, QByteArrayLiteral("given")},
        {ValueRole, QByteArrayLiteral("value")},
        {NotesRole, QByteArrayLiteral("notes")},
        {WrongRole, QByteArrayLiteral("wrong")},
    };
}

void CellModel::refresh(const std::vector<int> &indices) {
    for (int cell : indices) {
        const QModelIndex changed = index(cell);
        emit dataChanged(changed, changed);
    }
}

void CellModel::refreshAll() {
    if (rowCount() > 0)
        emit dataChanged(index(0), index(rowCount() - 1));
}
