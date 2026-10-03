#pragma once

#include <QRandomGenerator>
#include <array>

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

    // Shuffles every piece past the first `visible` again from `seed`, each
    // within its own bag, so nothing repeats inside a bag afterwards either;
    // what is dealt beyond the pieces already queued follows the new seed.
    void reseedHidden(int visible, quint32 seed);

private:
    void refill();
    PieceType &at(int ahead) { return m_queue[size_t((m_head + ahead) % kCapacity)]; }

    // Refilled a whole bag at a time whenever fewer than kLookahead are left,
    // so it never holds more than two bags. A fixed ring rather than a deque:
    // an agent's search copies the game thousands of times a piece, and a
    // copy that allocates nothing is several times cheaper.
    static constexpr int kCapacity = 2 * kPieceCount;

    QRandomGenerator m_rng;
    std::array<PieceType, size_t(kCapacity)> m_queue {};
    int m_head = 0;
    int m_size = 0;
};
