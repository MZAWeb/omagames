#include "cards.h"

int Card::value() const {
    return rank > 10 ? 10 : rank;
}

QString Card::rankText() const {
    switch (rank) {
    case 1: return QStringLiteral("A");
    case 11: return QStringLiteral("J");
    case 12: return QStringLiteral("Q");
    case 13: return QStringLiteral("K");
    default: return QString::number(rank);
    }
}

QString Card::suitGlyph() const {
    switch (suit) {
    case Suit::Clubs: return QStringLiteral("♣");
    case Suit::Diamonds: return QStringLiteral("♦");
    case Suit::Hearts: return QStringLiteral("♥");
    case Suit::Spades: return QStringLiteral("♠");
    }
    return QString();
}

Shoe::Shoe(int decks, quint32 seed)
    : m_decks(decks),
      m_seed(seed ? seed : QRandomGenerator::global()->generate()),
      m_rng(m_seed) {
    shuffle();
}

void Shoe::shuffle() {
    m_cards.clear();
    m_cards.reserve(m_decks * 52);
    for (int d = 0; d < m_decks; ++d)
        for (int s = 0; s < 4; ++s)
            for (int r = 1; r <= 13; ++r)
                m_cards.append({r, static_cast<Suit>(s)});
    // Fisher-Yates driven by our own generator so a seed fully determines the order.
    for (int i = m_cards.size() - 1; i > 0; --i)
        std::swap(m_cards[i], m_cards[m_rng.bounded(i + 1)]);
    m_next = 0;
}

Card Shoe::draw() {
    if (m_next >= m_cards.size())
        shuffle();
    return m_cards[m_next++];
}
