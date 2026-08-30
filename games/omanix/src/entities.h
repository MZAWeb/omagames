#pragma once

#include <QPoint>

class Field;

enum class Direction { None, Up, Down, Left, Right };

QPoint delta(Direction direction);
Direction opposite(Direction direction);

// A ball drifts diagonally through the open sea and bounces off claimed
// ground; the trail is passable, and deadly to whoever cut it.
struct Ball {
    QPoint pos;
    QPoint dir;

    void step(const Field &field);
};

// A chaser crawls along the edge of the claimed ground — Xonix's border
// enemy. It keeps the sea on its right, which walks the opening frame
// clockwise, follows every edge a claim adds and turns around at a dead end.
// `dir` is an axis-aligned unit vector.
struct Chaser {
    QPoint pos;
    QPoint dir;

    void step(const Field &field);

private:
    void walkBackToTheEdge(const Field &field);
};

// The marker. `held` is the key the player is holding right now; `queued`
// remembers a tap that has not produced its one cell of movement yet.
struct Player {
    QPoint pos;
    Direction dir = Direction::None;
    Direction held = Direction::None;
    bool queued = false;
    bool onTrail = false;
};
