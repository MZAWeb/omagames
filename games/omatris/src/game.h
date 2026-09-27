#pragma once

#include <QPoint>
#include <algorithm>
#include <optional>
#include <vector>

#include "bag.h"
#include "board.h"
#include "dealtstack.h"
#include "lockdelay.h"
#include "rules.h"
#include "scoring.h"

enum class Phase : quint8 { Playing, GameOver, Finished };

// One thing that happened, in order, so a renderer can animate it.
struct Event {
    enum Type { Locked, LinesCleared, LevelUp, Held, TopOut, Finished };

    explicit Event(Type type) : type(type) {}

    Type type;
    std::vector<int> cells;  // board indices of the piece that locked
    std::vector<int> rows;   // rows that are clearing
    ClearInfo clear;
    QPoint at;  // the locked piece's middle, for a popup
    int level = 0;
};

// The whole game as a deterministic state machine: every tick() advances one
// sixtieth of a second from an explicit seed and reports what happened. No
// timers and no QObject: the bridge paces it, the tests drive it directly.
// Auto-shift belongs to the bridge, which knows about key presses; the engine
// only ever moves one cell at a time.
class Game {
public:
    // Pieces enter with two spare rows above them, which is as far as a wall
    // kick can lift one.
    static constexpr int kSpawnRow = Board::kHiddenRows - 2;

    Game(Mode mode, quint32 seed);

    Mode mode() const { return m_mode; }
    Phase phase() const { return m_phase; }
    bool paused() const { return m_paused; }
    int score() const { return m_scoring.score(); }
    int lines() const { return m_scoring.lines(); }
    int level() const { return m_scoring.level(); }
    // The level gravity follows: Sprint and Zen keep the first level's speed.
    int gravityLevel() const { return m_params.gravityRamps ? level() : Rules::kFirstLevel; }
    int lineGoal() const { return m_params.lineGoal; }
    int linesLeft() const { return m_params.lineGoal > 0 ? std::max(0, m_params.lineGoal - lines()) : 0; }
    // Challenge: how many rows the dealt stack covered, and how many of them
    // are still on the board. Both 0 in every other mode.
    int dealtRows() const { return m_stack ? m_stack->dealt() : 0; }
    int dealtRowsLeft() const { return m_stack ? m_stack->rowsLeft(m_clearingRows) : 0; }
    // The board rows that still belong to it, rows mid-clear included, so a
    // renderer can mark them.
    const std::vector<int> &dealtStack() const;
    // Difficulty::rate() of the board as it stood after the last piece
    // settled, lines cleared; 0 outside Challenge.
    int difficulty() const { return m_stack ? m_stack->difficulty() : 0; }
    int ticks() const { return m_ticks; }
    int elapsedMs() const { return m_ticks * 1000 / Rules::kTicksPerSecond; }
    int combo() const { return m_scoring.combo(); }
    bool backToBack() const { return m_scoring.backToBack(); }

    const Board &board() const { return m_board; }
    bool hasPiece() const { return m_hasPiece; }
    const Placement &piece() const { return m_piece; }
    // Where the piece would land if it fell now.
    Placement ghost() const;
    PieceType heldPiece() const { return m_hold; }
    bool holdAvailable() const { return !m_holdUsed; }
    std::vector<PieceType> nextQueue() const;
    // Rows waiting out the clear flash; empty the rest of the time.
    const std::vector<int> &clearingRows() const { return m_clearingRows; }
    int lockTicks() const { return m_lockDelay.ticks(); }
    int lockResets() const { return m_lockDelay.resets(); }

    void setPaused(bool paused) { m_paused = paused; }
    bool moveLeft() { return shift(-1); }
    bool moveRight() { return shift(1); }
    // As far as the piece goes towards dx, charged as a single move: an
    // instant auto-repeat is one slide, not a cell-by-cell spend of the
    // lock-delay allowance.
    bool slide(int dx);
    // +1 clockwise, -1 counter-clockwise.
    bool rotate(int quarters);
    void setSoftDrop(bool on) { m_softDrop = on; }
    // A multiple of gravity, or Handling::kInstantSoftDrop (0) for straight
    // to the floor.
    int softDropFactor() const { return m_softDropFactor; }
    void setSoftDropFactor(int factor) { m_softDropFactor = factor; }
    std::vector<Event> hardDrop();
    std::vector<Event> hold();
    std::vector<Event> tick();

    // Scenario hooks for tests: the rules never call these.
    Board &mutableBoard() { return m_board; }
    void placePiece(const Placement &placement);

private:
    bool playable() const { return m_phase == Phase::Playing && !m_paused; }
    // Sprint's forty lines, or the last row of a Challenge's stack cleared.
    bool goalReached() const;
    bool shift(int dx);
    qint64 gravityThisTick() const;
    bool grounded() const;
    Spin detectSpin(int kickIndex) const;
    void applyGravity();
    void lockPiece(std::vector<Event> &events);
    void finishClear(std::vector<Event> &events);
    void spawnNext(std::vector<Event> &events);
    void spawnPiece(PieceType type, std::vector<Event> &events);
    void topOut(std::vector<Event> &events);

    Mode m_mode;
    ModeParams m_params;
    Bag m_bag;
    Board m_board;
    Placement m_piece;
    LockDelay m_lockDelay;
    PieceType m_hold = PieceType::None;
    Phase m_phase = Phase::Playing;
    std::vector<int> m_clearingRows;
    // Challenge's mess; empty in every other mode.
    std::optional<DealtStack> m_stack;
    qint64 m_gravity = 0;
    Scoring m_scoring;
    int m_ticks = 0;
    int m_clearTicks = 0;
    int m_softDropFactor = Rules::kSoftDropFactor;
    Spin m_spin = Spin::None;
    bool m_hasPiece = false;
    bool m_holdUsed = false;
    bool m_softDrop = false;
    bool m_paused = false;
};
