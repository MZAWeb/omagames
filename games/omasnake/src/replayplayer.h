#pragma once

#include <QJsonObject>
#include <QString>
#include <optional>
#include <vector>

#include "game.h"

// An agent's game played back: replay/v1 (common/env/replay.h) as Omasnake
// reads it. The replay is the seed and every turn the agent's game made,
// each on the tick it was made, so dealing the same game and making the same
// turns is the whole of playing it.
//
// A Snake agent decides once a move and the game runs on its clock between
// decisions, so playing back is simply a tick at a time: a turn rides along
// with the tick after it, and the app shows a tick a frame at real speed.
class ReplayPlayer {
public:
    // Nothing, and the reason, unless it is an Omasnake replay of these
    // rules with walls, a speed and turns this game knows.
    static std::optional<ReplayPlayer> load(const QJsonObject &json, QString *error);
    static std::optional<ReplayPlayer> loadFile(const QString &path, QString *error);

    Mode mode() const { return m_mode; }
    Difficulty difficulty() const { return m_difficulty; }
    quint32 seed() const { return m_seed; }
    // Who played it; empty when the replay does not say.
    const QString &agent() const { return m_agent; }

    // The game as the replay's was dealt, before any turn.
    Game deal() const;

    bool done() const { return m_next >= m_calls.size(); }

    // The turns waiting on the next tick, then that tick.
    std::vector<Event> tick(Game &game);
    // Ticks until the snake has moved once more, or the replay ends.
    std::vector<Event> nextMove(Game &game);
    // Back to the first turn; deal() a fresh game to go with it.
    void rewind() { m_next = 0; }

private:
    // A turn, or nothing for a tick.
    std::vector<std::optional<Direction>> m_calls;
    Mode m_mode = Mode::Classic;
    Difficulty m_difficulty = Difficulty::Normal;
    quint32 m_seed = 0;
    QString m_agent;
    size_t m_next = 0;
};
