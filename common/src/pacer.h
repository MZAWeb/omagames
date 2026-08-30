#pragma once

#include <QObject>
#include <QTimer>

#include <functional>

namespace OmaGames {

// The timer a bridge paces its engine with: a falling piece, a moving snake,
// a dealer drawing cards. The interval is a property so a test can set it to
// 0, and 0 means "no timer" — a Repeating pacer simply stays stopped and its
// test calls step() itself, a SingleShot pacer runs the step synchronously.
// Either way a headless test drives the game and stays deterministic.
//
// The step itself stays in the bridge; the pacer only decides when it runs.
class Pacer : public QObject {
    Q_OBJECT

public:
    enum Mode {
        // Fires every `interval` ms for as long as the bridge says to run.
        Repeating,
        // Fires once; the step decides whether to schedule the next one.
        SingleShot,
    };

    Pacer(Mode mode, std::function<void()> step, QObject *parent = nullptr);

    int interval() const { return m_interval; }
    // True when the value actually changed, so the bridge knows whether to
    // emit. Setting the interval never starts or stops anything: when to run
    // is the bridge's call, and it says so with setRunning() or runIn().
    bool setInterval(int ms);

    void setTimerType(Qt::TimerType type) { m_timer.setTimerType(type); }

    // Repeating: run while `running` and the interval is positive, stop
    // otherwise. This is the whole of a bridge's syncTimer().
    void setRunning(bool running);
    // SingleShot: fire once in `ms`, or run the step right now when `ms` is 0.
    void runIn(int ms);
    void stop() { m_timer.stop(); }
    bool isActive() const { return m_timer.isActive(); }

private:
    QTimer m_timer {this};
    std::function<void()> m_step;
    int m_interval = 0;
};

}  // namespace OmaGames
