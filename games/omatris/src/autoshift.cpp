#include "autoshift.h"

void AutoShift::press(int direction) {
    m_direction = direction;
    m_ticks = 0;
}

void AutoShift::release(int direction) {
    if (m_direction == direction)
        m_direction = 0;
}

void AutoShift::clear() {
    m_direction = 0;
    m_ticks = 0;
}

int AutoShift::tick() {
    if (m_direction == 0)
        return 0;
    ++m_ticks;
    // The delay is waited out once; after it, every repeat interval fires.
    if (m_ticks < kDelayTicks || (m_ticks - kDelayTicks) % kRepeatTicks != 0)
        return 0;
    return m_direction;
}
