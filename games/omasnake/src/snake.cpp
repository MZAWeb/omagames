#include "snake.h"

QPoint delta(Direction direction) {
    switch (direction) {
    case Direction::Up:
        return {0, -1};
    case Direction::Down:
        return {0, 1};
    case Direction::Left:
        return {-1, 0};
    case Direction::Right:
        break;
    }
    return {1, 0};
}

Direction opposite(Direction direction) {
    switch (direction) {
    case Direction::Up:
        return Direction::Down;
    case Direction::Down:
        return Direction::Up;
    case Direction::Left:
        return Direction::Right;
    case Direction::Right:
        break;
    }
    return Direction::Left;
}

Snake::Snake(QPoint head, int length, Direction heading) : m_heading(heading) {
    const QPoint back = delta(opposite(heading));
    for (int i = 0; i < length; ++i)
        m_body.push_back(head + back * i);
}

void Snake::turn(Direction direction) {
    // Measured against where the snake will be pointing once the turns
    // already queued have been taken, not against its heading right now.
    const Direction last = m_queue.empty() ? m_heading : m_queue.back();
    if (direction == last || direction == opposite(last))
        return;
    if (int(m_queue.size()) >= kMaxQueuedTurns)
        return;
    m_queue.push_back(direction);
}

void Snake::takeTurn() {
    if (m_queue.empty())
        return;
    m_heading = m_queue.front();
    m_queue.pop_front();
}

QPoint Snake::nextCell() const {
    return head() + delta(m_heading);
}

void Snake::step(QPoint head, bool grow) {
    m_body.push_front(head);
    if (!grow)
        m_body.pop_back();
}

bool Snake::occupies(QPoint p, bool ignoreTail) const {
    const int last = int(m_body.size()) - (ignoreTail ? 1 : 0);
    for (int i = 0; i < last; ++i) {
        if (m_body[size_t(i)] == p)
            return true;
    }
    return false;
}

void Snake::reset(const std::deque<QPoint> &body, Direction heading) {
    m_body = body;
    m_heading = heading;
    m_queue.clear();
}
