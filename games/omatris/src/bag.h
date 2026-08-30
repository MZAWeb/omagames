#pragma once

#include <QRandomGenerator>
#include <deque>

#include "piece.h"

// The guideline randomiser: the seven tetrominoes are dealt as shuffled bags
// of seven, so a piece is never more than twelve pieces away and no bag ever
// repeats one. Seeded explicitly, so a game replays exactly.
class Bag {
public:
    // Kept far enough ahead that the next queue can always be read off.
    static constexpr int kLookahead = kPieceCount + 1;

    explicit Bag(quint32 seed);

    PieceType take();
    // `ahead` 0 is the piece take() would hand out next.
    PieceType peek(int ahead) const;

private:
    void refill();

    QRandomGenerator m_rng;
    std::deque<PieceType> m_queue;
};
