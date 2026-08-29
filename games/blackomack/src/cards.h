#pragma once

#include <QRandomGenerator>
#include <QString>
#include <QVector>

enum class Suit { Clubs, Diamonds, Hearts, Spades };

struct Card {
    int rank = 0;   // 1 = Ace .. 13 = King
    Suit suit = Suit::Clubs;

    int value() const;           // blackjack value: 10 for faces, 1 for an ace
    bool isAce() const { return rank == 1; }
    QString rankText() const;    // "A", "2".."10", "J", "Q", "K"
    QString suitGlyph() const;   // ♣ ♦ ♥ ♠
    bool isRed() const { return suit == Suit::Diamonds || suit == Suit::Hearts; }
    bool operator==(const Card &o) const { return rank == o.rank && suit == o.suit; }
};

// Multi-deck shoe. Seeded so tests and bot "mistakes" are reproducible; a
// seed of 0 picks a random one. Reshuffles itself when fewer than a quarter of
// the cards remain, mimicking a cut card.
class Shoe {
public:
    explicit Shoe(int decks = 6, quint32 seed = 0);

    Card draw();
    int remaining() const { return m_cards.size() - m_next; }
    bool needsShuffle() const { return remaining() < m_cards.size() / 4; }
    void shuffle();
    quint32 seed() const { return m_seed; }

private:
    int m_decks;
    quint32 m_seed;
    QRandomGenerator m_rng;
    QVector<Card> m_cards;
    int m_next = 0;
};
