#include "bag.h"

#include <algorithm>

Bag::Bag(quint32 seed) : m_rng(seed) {
    refill();
}

PieceType Bag::take() {
    const PieceType piece = at(0);
    m_head = (m_head + 1) % kCapacity;
    --m_size;
    refill();
    return piece;
}

PieceType Bag::peek(int ahead) const {
    return m_queue[size_t((m_head + ahead) % kCapacity)];
}

void Bag::reseedHidden(int visible, quint32 seed) {
    m_rng.seed(seed);
    // Whole bags are appended and taken from the front, so the queue is the
    // tail of one bag followed by whole ones: the boundaries fall every seven
    // places counted from the back.
    for (int end = m_size; end > 0; end -= kPieceCount) {
        const int begin = std::max({end - kPieceCount, visible, 0});
        for (int i = end - 1; i > begin; --i)
            std::swap(at(i), at(begin + int(m_rng.bounded(quint32(i - begin + 1)))));
    }
}

void Bag::refill() {
    while (m_size < kLookahead) {
        PieceType bag[kPieceCount];
        for (int i = 0; i < kPieceCount; ++i)
            bag[i] = PieceType(i);
        // Fisher-Yates from the seeded generator: the shuffle is the only
        // randomness in the whole game.
        for (int i = kPieceCount - 1; i > 0; --i)
            std::swap(bag[i], bag[m_rng.bounded(quint32(i + 1))]);
        for (PieceType piece : bag)
            at(m_size++) = piece;
    }
}
