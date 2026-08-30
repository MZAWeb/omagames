#pragma once

#include <QPoint>
#include <QRandomGenerator>
#include <unordered_set>

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
// A chaser that catches itself going round in circles — the fate of anything
// crawling a pocket of sea the player has fenced off — leaves that boundary
// and walks over the ground to another region's edge.
// `dir` is an axis-aligned unit vector.
struct Chaser {
    // Steps spent crawling without meeting a new cell before the chaser gives
    // up on the stretch it is on: long enough to walk a deep spur down and
    // back, short enough that pacing a short one does not last.
    static constexpr int kStaleSteps = 128;

    Chaser() = default;
    Chaser(QPoint start, QPoint heading) : pos(start), dir(heading) {}

    QPoint pos;
    QPoint dir;
    // The (cell, heading) pairs crawled since the last re-target. Meeting one
    // twice means the chaser has come full circle.
    std::unordered_set<int> track;
    // Steps since `track` last learned a cell it had not seen.
    int stale = 0;
    // Where the chaser is walking to over the ground; an invalid point while
    // it is crawling a boundary.
    QPoint target {-1, -1};

    void step(const Field &field, QRandomGenerator &rng);
    // A claim moves the boundary under the chaser's feet, and everything the
    // track remembers is about the old one.
    void forgetTrack();

private:
    bool crawl(const Field &field, QRandomGenerator &rng);
    void remember(const Field &field);
    void retarget(const Field &field);
    QPoint elsewhere(const Field &field) const;
    void walkToTarget(const Field &field, QRandomGenerator &rng);
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
