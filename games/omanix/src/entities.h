#pragma once

#include <QPoint>
#include <QRandomGenerator>

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
// enemy. It follows every edge a claim adds and turns back only at a dead
// end; where the boundary branches it takes one of the ways on at random, so
// which way it goes is the game's seed rather than a fixed side of the sea.
// `dir` is an axis-aligned unit vector.
struct Chaser {
    Chaser() = default;
    Chaser(QPoint start, QPoint heading) : pos(start), dir(heading) {}

    QPoint pos;
    QPoint dir;

    void step(const Field &field, QRandomGenerator &rng);

private:
    void crawl(const Field &field, QRandomGenerator &rng);
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
