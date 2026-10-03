#pragma once

#include <optional>
#include <vector>

#include "calls.h"

// One place the falling piece can come to rest, how to get it there, and
// what locking it there would do.
struct Landing {
    // The held piece lands (or the next one, with nothing held) instead of
    // the falling one; `calls` then start with the hold.
    bool hold = false;
    // Where the piece rests, still unlocked.
    Placement placement;
    // What the lock would score as; only a T that turned last can spin.
    Spin spin = Spin::None;
    // From the game as it stands to the piece at rest; a hard drop locks it.
    std::vector<Call> calls;
    // What the hard drop that follows does: the lines it clears, whether it
    // ends the run, and the board once those lines are gone.
    int lines = 0;
    bool toppedOut = false;
    Board afterstate;
};

// Where a piece can go, found by playing copies of the game rather than by a
// second copy of the rules: every kick, the lock delay and T-spin detection
// are the engine's own.
//
// The moves are a shift, a turn either way, and a "sonic drop": one tick of
// an instant soft drop, which is what a player with Instant soft drop does
// with one tap of the key. The game must play with an instant soft drop for
// the calls to mean the same thing.
//
// `ticksPerInput` is how long each key press takes. At 0 no time passes
// between them: a player of infinite speed, for whom gravity never matters.
// Above 0, that many ticks pass after every press (a sonic drop's own tick
// included), so the piece falls and the lock delay runs while it is being
// moved, as for a human: at high gravity a piece can no longer be carried
// over a tall stack, and the landings it can't reach in time aren't offered.
// A landing is taken the moment the piece comes to rest there, since a hard
// drop needs no wait.
namespace Placements {

// Every distinct resting place, by the cells the piece would fill and how it
// would spin, each with its shortest sequence of calls; the held piece's
// too, when `withHold` and the game allows a hold. Breadth-first, so the
// order is the same every time for the same game.
std::vector<Landing> find(const Game &game, bool withHold, int ticksPerInput = 0);

// The landing a plain hard drop reaches: turned `rotation` quarter turns
// clockwise at the top, slid until its leftmost cell is in `column`, then
// dropped straight down. Nothing when it cannot get there.
std::optional<Landing> drop(const Game &game, bool hold, int rotation, int column, int ticksPerInput = 0);

// The leftmost column and the lowest row a placement fills.
int leftColumn(const Placement &placement);
int bottomRow(const Placement &placement);

}  // namespace Placements
