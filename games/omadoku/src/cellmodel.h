#pragma once

#include <QAbstractListModel>

#include <vector>

class SudokuBoard;

// The 81 cells as a list model so QML delegates can bind to one cell each and
// only the touched rows repaint.
class CellModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role {
        GivenRole = Qt::UserRole + 1,  // part of the puzzle, never editable
        ValueRole,                     // 0 = empty
        NotesRole,                     // 9-bit mask of pencil marks
        WrongRole,                     // flagged by the active check mode
    };

    explicit CellModel(const SudokuBoard *board, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void refresh(const std::vector<int> &indices);
    void refreshAll();

private:
    const SudokuBoard *m_board;
};
