// OmasnakeGame watching a recorded game instead of being played: what
// `--replay <file>` opens, or several of them to step between. The player's keys are ignored and nothing is
// ranked; the walls, speed and best shown are the recorded game's.
#include "omasnakegame.h"

bool OmasnakeGame::loadReplays(const QStringList &paths, QString *error) {
    if (paths.isEmpty()) {
        *error = QStringLiteral("no replay to watch");
        return false;
    }
    // All of them up front, so a bad one is said before anything opens
    // rather than when it is stepped onto.
    for (const QString &path : paths) {
        if (!ReplayPlayer::loadFile(path, error))
            return false;
    }
    m_playlist = OmaGames::ReplayPlaylist(paths);
    m_pace = OmaGames::ReplayPace();
    return watch(m_playlist.current(), error);
}

bool OmasnakeGame::watch(const QString &path, QString *error) {
    auto player = ReplayPlayer::loadFile(path, error);
    if (!player)
        return false;
    m_replay = std::move(player);
    m_pace.restart();
    show(std::make_unique<Game>(m_replay->deal()));
    emit replayChanged();
    return true;
}

void OmasnakeGame::nextReplay() {
    QString error;
    // A file gone since it was checked leaves the one on screen there.
    if (m_replay && m_playlist.next() && !watch(m_playlist.current(), &error))
        m_playlist.previous();
}

void OmasnakeGame::previousReplay() {
    QString error;
    if (m_replay && m_playlist.previous() && !watch(m_playlist.current(), &error))
        m_playlist.next();
}

void OmasnakeGame::restartReplay() {
    if (!m_replay)
        return;
    m_replay->rewind();
    m_pace.restart();
    show(std::make_unique<Game>(m_replay->deal()));
    emit replayChanged();
}

void OmasnakeGame::endReplay() {
    if (!m_replay)
        return;
    m_replay.reset();
    m_playlist = {};
    m_pace = OmaGames::ReplayPace();
    emit replayChanged();
}

void OmasnakeGame::setReplaySpeed(int speed) {
    if (m_replay && m_pace.setSpeed(speed))
        emit replayChanged();
}

void OmasnakeGame::replayFrame() {
    const int ticks = m_pace.nextFrame();
    if (ticks == 0)
        return;
    playReplay([this, ticks]() {
        std::vector<Event> events;
        for (int i = 0; i < ticks && !m_replay->done() && m_game->phase() == Phase::Playing; ++i) {
            const std::vector<Event> tick = m_replay->tick(*m_game);
            events.insert(events.end(), tick.begin(), tick.end());
        }
        return events;
    });
}

void OmasnakeGame::replayNextMove() {
    if (!m_replay || !m_game || m_game->phase() != Phase::Playing || m_replay->done())
        return;
    // A paused game ignores every tick, so it is let go for exactly one move
    // and held again.
    const bool paused = m_game->paused();
    m_game->setPaused(false);
    playReplay([this]() { return m_replay->nextMove(*m_game); });
    m_game->setPaused(paused);
}

void OmasnakeGame::playReplay(const std::function<std::vector<Event>()> &calls) {
    advance(calls);
    if (m_replay && m_replay->done()) {
        emit replayChanged();
        syncTimer();
    }
}
