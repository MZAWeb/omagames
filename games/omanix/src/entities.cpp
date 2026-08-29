#include "entities.h"

#include "field.h"

namespace {

// Reflect a diagonal mover Xonix-style: whichever axis is blocked flips, and
// a blocker sitting exactly on the corner flips both. If the way is still
// shut afterwards the mover stays put for this step.
template <typename Blocked>
void bounce(QPoint &pos, QPoint &dir, Blocked blocked) {
    const bool wallX = blocked({pos.x() + dir.x(), pos.y()});
    const bool wallY = blocked({pos.x(), pos.y() + dir.y()});
    if (wallX)
        dir.setX(-dir.x());
    if (wallY)
        dir.setY(-dir.y());
    if (!wallX && !wallY && blocked(pos + dir))
        dir = -dir;
    if (!blocked(pos + dir))
        pos += dir;
}

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

void Ball::step(const Field &field) {
    bounce(pos, dir, [&field](QPoint p) { return !field.contains(p) || field.at(p) == Cell::Claimed; });
}

void Chaser::step(const Field &field) {
    bounce(pos, dir, [&field](QPoint p) { return !field.contains(p) || field.at(p) != Cell::Claimed; });
}
