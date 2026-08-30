#pragma once

#include <QJsonObject>
#include <QPoint>
#include <QtGlobal>
#include <optional>
#include <vector>

enum class CellState : quint8 { Hidden, Flagged, Revealed };

// Ready: no reveal yet (mines may or may not be placed). Playing: the first
// cell is open and the clock should be running. Won/Lost are terminal.
enum class Status { Ready, Playing, Won, Lost };

struct Cell {
    CellState state = CellState::Hidden;
    bool mine = false;
    // Adjacent mines, valid once mines are placed.
    quint8 adjacent = 0;
};

// What one action did, so the UI can animate it. `revealed` is in cascade
// order (breadth-first from the clicked cell) so a zero region ripples
// outwards. A no-op leaves every list empty and `toggled` at -1.
struct MoveResult {
    std::vector<int> revealed;
    // The cell whose flag changed (toggleFlag only).
    int toggled = -1;
    // The mine that was hit, or -1.
    int exploded = -1;
    // Every flag standing on a safe cell, filled on a loss.
    std::vector<int> wrongFlags;
    Status status = Status::Ready;

    bool won() const { return status == Status::Won; }
    bool lost() const { return status == Status::Lost; }
    bool changed() const { return !revealed.empty() || toggled >= 0 || exploded >= 0; }
};

// The minefield and its rules: reveal, flag, chord, win and loss. Cells are
// addressed by (x, y) or by index `y * width + x`; every list of cells in
// the API uses indices.
//
// Mines are not placed at construction. They are placed either explicitly
// (`placeMines`, what the Generator does after proving the board solvable)
// or lazily on the first reveal from the board's own seed, which keeps the
// classic guarantee that the first click is safe and opens a zero: the 3×3
// around it holds no mine. QtCore only, copyable, no timers: elapsed time is
// the bridge's business, counted from the reveal that moves the status from
// Ready to Playing.
class Board {
public:
    static constexpr int kMinSize = 1;
    static constexpr int kMaxSize = 99;

    // `mines` is clamped to `cellCount() - 1`.
    Board(int width, int height, int mines, quint32 seed = 0);

    int width() const { return m_width; }
    int height() const { return m_height; }
    int mineCount() const { return m_mines; }
    int cellCount() const { return m_width * m_height; }
    // Cells that must be revealed to win.
    int safeCount() const { return cellCount() - m_mines; }
    quint32 seed() const { return m_seed; }

    int index(QPoint p) const { return p.y() * m_width + p.x(); }
    QPoint point(int index) const { return {index % m_width, index / m_width}; }
    bool contains(QPoint p) const;
    // The up-to-eight cells around `index`, in reading order.
    std::vector<int> neighbours(int index) const;

    const Cell &cell(int index) const { return m_cells[size_t(index)]; }
    const Cell &cell(QPoint p) const { return cell(index(p)); }
    const std::vector<Cell> &cells() const { return m_cells; }

    Status status() const { return m_status; }
    bool minesPlaced() const { return m_minesPlaced; }
    int revealedCount() const { return m_revealed; }
    int flagCount() const { return m_flags; }
    // Mines minus flags; goes negative when the player over-flags.
    int remainingMines() const { return m_mines - m_flags; }
    // Indices of every mine; empty until mines are placed.
    std::vector<int> mines() const;

    // Random placement from `seed`, keeping the 3×3 around `firstClick`
    // clear (just the cell itself if the board is too small for that).
    void placeMines(quint32 seed, QPoint firstClick);
    // Explicit placement (generator, tests). Ignored once placed.
    void placeMines(const std::vector<int> &mines);

    // Opens a hidden cell (placing mines first if needed). A mine loses; a
    // zero cascades. Flagged, revealed and out-of-range cells are no-ops.
    MoveResult reveal(QPoint p);
    // Flags a hidden cell or clears its flag; ignored on revealed cells and
    // once the game is over.
    MoveResult toggleFlag(QPoint p);
    // On a revealed number whose adjacent flags equal it: reveals every other
    // hidden neighbour. A wrong flag means a mine is among them, which loses.
    MoveResult chord(QPoint p);

    // Whole state for persistence (`state/v1`), including mines.
    QJsonObject toJson() const;
    static std::optional<Board> fromJson(const QJsonObject &json);

private:
    void computeAdjacent();
    void cascade(int start, MoveResult &result);
    void lose(int exploded, MoveResult &result);
    void finish(MoveResult &result);

    int m_width;
    int m_height;
    int m_mines;
    quint32 m_seed;
    std::vector<Cell> m_cells;
    bool m_minesPlaced = false;
    Status m_status = Status::Ready;
    int m_revealed = 0;
    int m_flags = 0;
};
