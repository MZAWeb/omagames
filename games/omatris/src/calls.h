#pragma once

#include <QString>
#include <optional>
#include <vector>

#include "game.h"

// What a player's keys come down to once the bridge has turned them into
// engine calls, plus the tick that lets time pass. A game is its seed and
// these, in order, so this is what an agent's game is recorded as and what
// the app plays back: replay/v1 writes each call as its token.
enum class Call : quint8 {
    Left,
    Right,
    RotateCW,
    RotateCCW,
    SoftDropOn,
    SoftDropOff,
    HardDrop,
    Hold,
    Tick,
};

namespace Calls {

// "L", "R", "CW", "CCW", "SD+", "SD-", "HD", "H"; a tick has no token of its
// own, since a replay counts ticks rather than spelling them out.
QString token(Call call);
std::optional<Call> fromToken(const QString &token);
// Makes the call the way the bridge would and returns what happened.
std::vector<Event> apply(Game &game, Call call);

}  // namespace Calls
