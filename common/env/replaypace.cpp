#include "replaypace.h"

namespace OmaGames {

namespace {

// Per speed, slowest first.
const int kQuarterBeatsPerFrame[ReplayPace::kSpeeds] = {1, 2, 4, 32};
const char *const kLabels[ReplayPace::kSpeeds] = {"¼×", "½×", "1×", "8×"};

}  // namespace

bool ReplayPace::setSpeed(int speed) {
    if (speed == m_speed || speed < 1 || speed > kSpeeds)
        return false;
    m_speed = speed;
    m_owed = 0;
    return true;
}

QString ReplayPace::label() const {
    return QString::fromUtf8(kLabels[m_speed - 1]);
}

int ReplayPace::nextFrame() {
    m_owed += kQuarterBeatsPerFrame[m_speed - 1];
    const int beats = m_owed / 4;
    m_owed %= 4;
    return beats;
}

}  // namespace OmaGames
