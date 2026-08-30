#pragma once

#include <QPoint>
#include <deque>

enum class Direction { Up, Down, Left, Right };

QPoint delta(Direction direction);
Direction opposite(Direction direction);

// The snake itself: the cells it stands on (head first), where it is going
// and the turns still waiting to be taken. It knows nothing about walls,
// food or score — the Game moves it and decides what the move means.
class Snake {
public:
    // Two turns may wait at once, so a quick double tap round a corner lands
    // both instead of losing the second one.
    static constexpr int kMaxQueuedTurns = 2;

    Snake(QPoint head, int length, Direction heading);

    const std::deque<QPoint> &body() const { return m_body; }
    QPoint head() const { return m_body.front(); }
    QPoint tail() const { return m_body.back(); }
    int length() const { return int(m_body.size()); }
    Direction heading() const { return m_heading; }
    int queuedTurns() const { return int(m_queue.size()); }

    // Queues a turn. Reversing onto its own neck, repeating the heading it
    // already has and anything past the queue's depth are all ignored.
    void turn(Direction direction);
    // Takes the next queued turn, if any: one turn per move.
    void takeTurn();
    // The cell straight ahead of the head; may be off the field.
    QPoint nextCell() const;
    // Moves onto `head`, dropping the tail unless the snake is growing.
    void step(QPoint head, bool grow);
    // Is `p` part of the snake? `ignoreTail` skips the cell the tail is about
    // to leave, which a non-growing snake is free to move onto.
    bool occupies(QPoint p, bool ignoreTail = false) const;

    // Scenario hook for tests; the rules never call it.
    void reset(const std::deque<QPoint> &body, Direction heading);

private:
    std::deque<QPoint> m_body;
    std::deque<Direction> m_queue;
    Direction m_heading;
};
