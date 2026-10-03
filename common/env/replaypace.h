#pragma once

#include <QString>

namespace OmaGames {

// How fast an app shows a replay: four speeds, keys 1 to 4, slowest first,
// at a quarter, half, the same and eight times one beat a frame. What a beat
// is belongs to the game: a tick for Snake, a tick or a visible move for
// Omatris. At the same speed, a game recorded in real time plays in real
// time.
class ReplayPace {
public:
    static constexpr int kSpeeds = 4;
    static constexpr int kRealSpeed = 3;

    explicit ReplayPace(int speed = kRealSpeed) : m_speed(speed) {}

    int speed() const { return m_speed; }
    // False, and nothing changed, for the speed it already has or one that
    // is not 1 to kSpeeds.
    bool setSpeed(int speed);
    // "¼×", "½×", "1×", "8×".
    QString label() const;
    // Past real speed, pieces and dots come and go faster than a popup can
    // rise and fade.
    bool fasterThanReal() const { return m_speed > kRealSpeed; }

    // The beats to show this frame: at the slow speeds, none on most frames.
    int nextFrame();
    // Forgets any part of a beat owed, for a replay starting over.
    void restart() { m_owed = 0; }

private:
    int m_speed;
    // Quarter beats owed from earlier frames.
    int m_owed = 0;
};

}  // namespace OmaGames
