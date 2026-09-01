#pragma once

#include <QPoint>
#include <QVector>

enum class Direction { Left, Right, Up, Down };

// A numbered tile on the grid. Ids are stable for the life of the board so a
// renderer can follow a tile across moves and animate the slide; a merge
// keeps the id of the tile that was slid onto and drops the other.
struct Tile {
    int id;
    int value;
    int row;
    int col;
};

struct MoveResult {
    bool moved = false;
    int scoreGained = 0;
};

// The 4x4 grid and the sliding rules, nothing else: no randomness (the Game
// spawns), no timers, no Qt GUI. Every tile slides as far as it can; equal
// tiles that collide merge into their sum, at most once per tile per move,
// pairs closest to the destination edge first.
class Board {
public:
    static constexpr int kSize = 4;

    MoveResult move(Direction direction);
    bool canMove(Direction direction) const;
    bool anyMoveAvailable() const;

    const QVector<Tile> &tiles() const { return m_tiles; }
    // Empty cells as QPoint(col, row): x is the column, y the row.
    QVector<QPoint> emptyCells() const;
    int valueAt(int row, int col) const;
    int idAt(int row, int col) const;
    int highestValue() const;

    // Places a fresh tile, replacing whatever occupies the cell. This is how
    // spawns and test fixtures get onto the board; the rules never call it.
    void put(int row, int col, int value);
    void clear();

private:
    int indexAt(int row, int col) const;

    QVector<Tile> m_tiles;
    int m_nextId = 1;
};
