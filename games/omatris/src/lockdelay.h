#pragma once

// How long a landed piece may still be moved before it locks, and how many
// moves can buy it more time. It knows rows, not the board: the game says
// where the piece's origin is and whether it rests on something, and asks
// when time is up.
//
// The timer only runs while the piece rests on something, and in mid-air it
// pauses rather than rewinds: a rotation that lifts the piece off the stack
// for a moment buys it no time it has not paid a reset for.
class LockDelay {
public:
    // A new piece, its origin on `row`, with the whole allowance.
    void start(int row);
    // The piece was shifted or turned; `row` is where its origin is now.
    void moved(int row, bool grounded);
    // Gravity took the piece down to `row`.
    void fell(int row);
    // One tick resting on the stack. True once the delay has run out.
    bool rest();

    int ticks() const { return m_ticks; }
    int resets() const { return m_resets; }

private:
    int m_ticks = 0;
    int m_resets = 0;
    // The deepest row the piece's origin has ever reached, so a kick that
    // bounces it down and back up cannot pass for falling.
    int m_lowestRow = 0;
    // Set once the piece has rested on something since it last fell to a new
    // lowest row: only then does a move spend part of the allowance.
    bool m_pending = false;
};
