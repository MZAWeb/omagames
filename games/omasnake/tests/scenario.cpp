#include "scenario.h"

#include <algorithm>

namespace Scenario {

void skipReady(Game &game) {
    for (int i = 0; i < Game::kReadyTicks; ++i)
        game.tick();
}

std::vector<Event> stepMove(Game &game) {
    std::vector<Event> all;
    const int ticks = game.moveTicks();
    for (int i = 0; i < ticks; ++i) {
        const std::vector<Event> events = game.tick();
        all.insert(all.end(), events.begin(), events.end());
    }
    return all;
}

bool has(const std::vector<Event> &events, Event::Type type) {
    return std::any_of(events.begin(), events.end(), [type](const Event &e) { return e.type == type; });
}

Event find(const std::vector<Event> &events, Event::Type type) {
    for (const Event &e : events) {
        if (e.type == type)
            return e;
    }
    return {Event::GameOver, {-1, -1}};
}

std::deque<QPoint> row(QPoint head, int length) {
    std::deque<QPoint> body;
    for (int i = 0; i < length; ++i)
        body.push_back(head - QPoint(i, 0));
    return body;
}

std::vector<QPoint> serpentine() {
    std::vector<QPoint> path;
    path.reserve(size_t(Game::kWidth * Game::kHeight));
    for (int y = 0; y < Game::kHeight; ++y) {
        for (int i = 0; i < Game::kWidth; ++i)
            path.push_back({y % 2 == 0 ? i : Game::kWidth - 1 - i, y});
    }
    return path;
}

std::deque<QPoint> bodyAlong(const std::vector<QPoint> &path, int count) {
    std::deque<QPoint> body;
    for (int i = 0; i < count; ++i)
        body.push_front(path[size_t(i)]);
    return body;
}

Direction headingFrom(QPoint from, QPoint to) {
    const QPoint step = to - from;
    if (step == QPoint(1, 0))
        return Direction::Right;
    if (step == QPoint(-1, 0))
        return Direction::Left;
    if (step == QPoint(0, -1))
        return Direction::Up;
    return Direction::Down;
}

}  // namespace Scenario
