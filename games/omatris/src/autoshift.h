#pragma once

// Delayed auto shift: what a left or right key does while it stays down. The
// engine only ever moves a piece one cell and knows nothing about keys, so the
// rule that turns a held key into a stream of moves lives here, driven one
// tick at a time so it can be tested without a bridge or a board.
class AutoShift {
public:
    // The wait before a held key starts repeating, and the gap between
    // repeats, in ticks of the sixty-a-second clock: ~167 ms, then ~33 ms.
    static constexpr int kDelayTicks = 10;
    static constexpr int kRepeatTicks = 2;

    // -1 left, +1 right, 0 nothing held.
    int direction() const { return m_direction; }

    // A key going down takes the shift over and starts the wait again. The
    // first move is the caller's: this only ever asks for the repeats.
    void press(int direction);
    // Only the key that holds the shift can let it go, so the one still down
    // after a roll from left to right keeps shifting.
    void release(int direction);
    // Nothing is held any more: a pause, a new game, the end of a run.
    void clear();
    // One tick of the clock: -1 or +1 when a repeat falls due now, else 0.
    int tick();

private:
    int m_direction = 0;
    int m_ticks = 0;
};
