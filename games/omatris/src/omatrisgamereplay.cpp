// OmatrisGame watching a recorded game instead of being played: what
// `--replay <file>` opens. The player's keys are ignored, nothing is ranked,
// and the replay drops pieces as fast as it was recorded dropping them.
#include "omatrisgame.h"

namespace {

// A placing agent's piece is about eight beats, so half speed shows three or
// four pieces a second.
constexpr int kDefaultSpeed = 2;

}  // namespace

bool OmatrisGame::loadReplay(const QString &path, QString *error) {
    auto player = ReplayPlayer::loadFile(path, error);
    if (!player)
        return false;
    m_replay = std::move(player);
    m_pace = OmaGames::ReplayPace(kDefaultSpeed);
    show(std::make_unique<Game>(m_replay->deal()));
    emit replayChanged();
    return true;
}

void OmatrisGame::restartReplay() {
    if (!m_replay)
        return;
    m_replay->rewind();
    m_pace.restart();
    show(std::make_unique<Game>(m_replay->deal()));
    emit replayChanged();
}

void OmatrisGame::endReplay() {
    if (!m_replay)
        return;
    m_replay.reset();
    m_pace = OmaGames::ReplayPace(kDefaultSpeed);
    emit replayChanged();
}

bool OmatrisGame::replayTooFastToRead() const {
    return m_replay && m_pace.fasterThanReal();
}

void OmatrisGame::setReplaySpeed(int speed) {
    if (m_replay && m_pace.setSpeed(speed))
        emit replayChanged();
}

void OmatrisGame::replayBeats() {
    const int beats = m_pace.nextFrame();
    if (beats == 0)
        return;
    playReplay([this, beats]() {
        std::vector<Event> events;
        for (int i = 0; i < beats && !m_replay->done() && m_game->phase() == Phase::Playing; ++i) {
            const std::vector<Event> beat = m_replay->beat(*m_game);
            events.insert(events.end(), beat.begin(), beat.end());
        }
        return events;
    });
}

void OmatrisGame::replayNextPiece() {
    if (!m_replay || !m_game || m_game->phase() != Phase::Playing || m_replay->done())
        return;
    // A paused game ignores every call, so it is let go for exactly one
    // piece and held again.
    const bool paused = m_game->paused();
    m_game->setPaused(false);
    playReplay([this]() { return m_replay->nextPiece(*m_game); });
    m_game->setPaused(paused);
}

void OmatrisGame::playReplay(const std::function<std::vector<Event>()> &calls) {
    const Snapshot before = snapshot();
    apply(calls());
    publish(before);
    emit frameChanged();
    if (m_replay && m_replay->done()) {
        emit replayChanged();
        syncTimer();
    }
}
