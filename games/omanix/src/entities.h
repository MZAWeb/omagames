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

// A chaser is the ball's mirror image: it drifts diagonally through claimed
// ground and bounces off the sea.
struct Chaser {
    QPoint pos;
    QPoint dir;

    void step(const Field &field);
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
