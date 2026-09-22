#include "autoshift.h"

#include <algorithm>

void AutoShift::setTiming(int delayTicks, int repeatTicks) {
    m_delayTicks = std::max(1, delayTicks);
    m_repeatTicks = std::max(0, repeatTicks);
}

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
    // The delay is waited out once; after it, every repeat interval fires,
    // and with no interval at all, every tick does.
    if (m_ticks < m_delayTicks)
        return 0;
    if (m_repeatTicks > 0 && (m_ticks - m_delayTicks) % m_repeatTicks != 0)
        return 0;
    return m_direction;
}
