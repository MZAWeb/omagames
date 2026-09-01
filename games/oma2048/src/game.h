#pragma once

#include <QJsonObject>
#include <QRandomGenerator>
#include <optional>

#include "board.h"

// One run of 2048: the board plus everything around it — spawning, score,
// the won-once flag, game over, and the single level of undo. Deterministic
// from the seed; no timers, no QObject, so tests drive it directly.
class Game {
public:
    static constexpr int kWinValue = 2048;

    explicit Game(quint32 seed);

    void newGame();
    // Applies the slide; a move that changes nothing is ignored entirely —
    // nothing spawns, no undo point is made — and reports false.
    bool move(Direction direction);
    bool canUndo() const { return m_undo.has_value() && !m_over; }
    bool undo();
    // "Keep going" on the win overlay: the run continues with the same score.
    void keepGoing() { m_keepPlaying = m_won; }

    const Board &board() const { return m_board; }
    int score() const { return m_score; }
    bool won() const { return m_won; }
    bool over() const { return m_over; }
    bool keepPlaying() const { return m_keepPlaying; }

    // The in-progress run for `state/v1`. Undo is deliberately not part of it:
    // a relaunch starts with nothing to take back.
    QJsonObject toJson() const;
    bool restore(const QJsonObject &json);

private:
    struct Snapshot {
        Board board;
        int score;
        bool won;
        bool keepPlaying;
    };

    void spawn();

    QRandomGenerator m_rng;
    Board m_board;
    int m_score = 0;
    bool m_won = false;
    bool m_keepPlaying = false;
    bool m_over = false;
    std::optional<Snapshot> m_undo;
};
