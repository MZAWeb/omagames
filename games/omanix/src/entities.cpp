#include "entities.h"

#include <cstdlib>
#include <vector>

#include "field.h"

namespace {

bool blocksBall(const Field &field, QPoint p) {
    return !field.contains(p) || field.at(p) == Cell::Claimed;
}

// Screen coordinates: y grows downward, so this turns a heading to its right.
QPoint clockwise(QPoint dir) {
    return {-dir.y(), dir.x()};
}

const QPoint kSteps[] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};

}  // namespace

QPoint delta(Direction direction) {
    switch (direction) {
    case Direction::Up:
        return {0, -1};
    case Direction::Down:
        return {0, 1};
    case Direction::Left:
        return {-1, 0};
    case Direction::Right:
        return {1, 0};
    case Direction::None:
        break;
    }
    return {0, 0};
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
        return Direction::Left;
    case Direction::None:
        break;
    }
    return Direction::None;
}

// Reflect Xonix-style: whichever axis is blocked flips, and a blocker sitting
// exactly on the corner flips both. If the way is still shut afterwards the
// ball stays put for this step.
void Ball::step(const Field &field) {
    const bool wallX = blocksBall(field, {pos.x() + dir.x(), pos.y()});
    const bool wallY = blocksBall(field, {pos.x(), pos.y() + dir.y()});
    if (wallX)
        dir.setX(-dir.x());
    if (wallY)
        dir.setY(-dir.y());
    if (!wallX && !wallY && blocksBall(field, pos + dir))
        dir = -dir;
    if (!blocksBall(field, pos + dir))
        pos += dir;
}

void Chaser::step(const Field &field) {
    if (!field.isEdge(pos)) {
        walkBackToTheEdge(field);
        return;
    }
    // A chaser dropped on the field has no heading yet; send it off to the
    // right and let the edge steer it from there.
    if (std::abs(dir.x()) + std::abs(dir.y()) != 1)
        dir = {1, 0};
    // Hug the sea: turn toward it first, then carry straight on, then away
    // from it, and only turn around when the edge dead-ends.
    const QPoint options[] = {clockwise(dir), dir, -clockwise(dir), -dir};
    for (QPoint option : options) {
        if (field.isEdge(pos + option)) {
            dir = option;
            pos += option;
            return;
        }
    }
}

// A claim can bury a chaser deep inside the ground it used to hug. Walk it
// back out along the shortest path over claimed cells; the fixed order the
// neighbours are visited in keeps that path deterministic.
void Chaser::walkBackToTheEdge(const Field &field) {
    if (!field.contains(pos) || field.at(pos) != Cell::Claimed)
        return;
    const int start = field.index(pos);
    std::vector<int> from(size_t(field.cellCount()), -1);
    std::vector<int> queue {start};
    from[size_t(start)] = start;
    for (size_t head = 0; head < queue.size(); ++head) {
        const int current = queue[head];
        const QPoint p = field.point(current);
        if (current != start && field.isEdge(p)) {
            int first = current;
            while (from[size_t(first)] != start)
                first = from[size_t(first)];
            dir = field.point(first) - pos;
            pos = field.point(first);
            return;
        }
        for (QPoint step : kSteps) {
            const QPoint n = p + step;
            if (!field.contains(n) || field.at(n) != Cell::Claimed)
                continue;
            const int next = field.index(n);
            if (from[size_t(next)] >= 0)
                continue;
            from[size_t(next)] = current;
            queue.push_back(next);
        }
    }
}
