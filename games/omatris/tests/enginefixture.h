#pragma once

#include <algorithm>
#include <initializer_list>

#include "game.h"

// What every engine suite needs to set a scene up and read the events back.
namespace EngineFixture {

constexpr quint32 kSeed = 20260830u;
constexpr int kBottom = Board::kHeight - 1;

inline int count(const std::vector<Event> &events, Event::Type type) {
    return int(std::count_if(events.begin(), events.end(), [type](const Event &e) { return e.type == type; }));
}

inline const Event *find(const std::vector<Event> &events, Event::Type type) {
    for (const Event &event : events) {
        if (event.type == type)
            return &event;
    }
    return nullptr;
}

// Fills a row solid except for the named columns.
inline void fillRow(Board &board, int y, std::initializer_list<int> gaps) {
    for (int x = 0; x < Board::kWidth; ++x) {
        if (std::find(gaps.begin(), gaps.end(), x) == gaps.end())
            board.set({x, y}, PieceType::L);
    }
}

inline void waitOutTheFlash(Game &game) {
    for (int i = 0; i < Rules::kClearDelayTicks; ++i)
        game.tick();
}

}  // namespace EngineFixture
