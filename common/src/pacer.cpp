#include "pacer.h"

namespace OmaGames {

Pacer::Pacer(Mode mode, std::function<void()> step, QObject *parent)
    : QObject(parent), m_step(std::move(step)) {
    m_timer.setSingleShot(mode == SingleShot);
    connect(&m_timer, &QTimer::timeout, this, [this]() { m_step(); });
}

bool Pacer::setInterval(int ms) {
    if (m_interval == ms)
        return false;
    m_interval = ms;
    return true;
}

void Pacer::setRunning(bool running) {
    if (running && m_interval > 0)
        m_timer.start(m_interval);
    else
        m_timer.stop();
}

void Pacer::runIn(int ms) {
    if (ms == 0)
        m_step();
    else
        m_timer.start(ms);
}

}  // namespace OmaGames
