#include "bag.h"

Bag::Bag(quint32 seed) : m_rng(seed) {
    refill();
}

PieceType Bag::take() {
    const PieceType piece = m_queue.front();
    m_queue.pop_front();
    refill();
    return piece;
}

PieceType Bag::peek(int ahead) const {
    return m_queue[size_t(ahead)];
}

void Bag::refill() {
    while (int(m_queue.size()) < kLookahead) {
        PieceType bag[kPieceCount];
        for (int i = 0; i < kPieceCount; ++i)
            bag[i] = PieceType(i);
        // Fisher-Yates from the seeded generator: the shuffle is the only
        // randomness in the whole game.
        for (int i = kPieceCount - 1; i > 0; --i)
            std::swap(bag[i], bag[m_rng.bounded(quint32(i + 1))]);
        for (PieceType piece : bag)
            m_queue.push_back(piece);
    }
}
