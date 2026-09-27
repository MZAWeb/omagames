#include "lockdelay.h"

#include <algorithm>

#include "rules.h"

void LockDelay::start(int row) {
    m_ticks = 0;
    m_resets = 0;
    m_pending = false;
    m_lowestRow = row;
}

void LockDelay::moved(int row, bool grounded) {
    // A kick that pushes the piece below anywhere it has been only raises the
    // mark: falling is what earns a fresh allowance, not turning.
    m_lowestRow = std::max(m_lowestRow, row);
    // Once the piece has landed, every move spends one of the allowance, and
    // only an unspent one restarts the timer. After that the timer runs out
    // however hard the keys are hammered.
    if (m_pending && m_resets < Rules::kMaxLockResets) {
        ++m_resets;
        m_ticks = 0;
    }
    if (grounded)
        m_pending = true;
}

void LockDelay::fell(int row) {
    // Reaching a row it has never been on before hands the piece a whole
    // fresh allowance; falling back onto one it has already visited does not.
    if (row > m_lowestRow)
        start(row);
}

bool LockDelay::rest() {
    m_pending = true;
    return ++m_ticks >= Rules::kLockDelayTicks;
}
