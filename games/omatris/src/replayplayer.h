#pragma once

#include <QJsonObject>
#include <QString>
#include <optional>
#include <vector>

#include "calls.h"

// An agent's game played back: replay/v1 (common/env/replay.h) as Omatris
// reads it. The replay is the seed and every call the agent's game made, so
// dealing the same game and making the same calls is the whole of playing it.
//
// It plays in beats, which is what the app shows one of per frame. A tick is
// a beat, a hard drop or a hold is, and so is any other input made while no
// time passes after another one: an agent placing pieces moves them with no
// time between its inputs, so each move gets a beat of its own and can be
// seen, while an agent pressing keys in real time plays at about real speed.
class ReplayPlayer {
public:
    // Nothing, and the reason, unless it is an Omatris replay of these rules
    // with a mode and calls this game knows.
    static std::optional<ReplayPlayer> load(const QJsonObject &json, QString *error);
    static std::optional<ReplayPlayer> loadFile(const QString &path, QString *error);

    Mode mode() const { return m_mode; }
    quint32 seed() const { return m_seed; }
    // Who played it; empty when the replay does not say.
    const QString &agent() const { return m_agent; }

    // The game as the replay's was dealt, before any call.
    Game deal() const;

    bool done() const { return m_next >= m_calls.size(); }
    // Calls made so far and in all, for a progress bar.
    int position() const { return int(m_next); }
    int length() const { return int(m_calls.size()); }

    // Makes calls on `game` up to and including the next beat.
    std::vector<Event> beat(Game &game);
    // Beats until a piece locks, or the replay ends.
    std::vector<Event> nextPiece(Game &game);
    // Back to the first call; deal() a fresh game to go with it.
    void rewind() { m_next = 0; }

private:
    bool isBeat(size_t index) const;

    Mode m_mode = Mode::Marathon;
    quint32 m_seed = 0;
    int m_softDropFactor = 0;
    QString m_agent;
    std::vector<Call> m_calls;
    size_t m_next = 0;
};
