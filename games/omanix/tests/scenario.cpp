#include "scenario.h"

#include <algorithm>

namespace Scenario {

void walk(Game &game, Direction direction, int cells, std::vector<Event> *events) {
    game.setDirection(direction);
    for (int i = 0; i < cells; ++i) {
        for (int t = 0; t < game.params().playerPeriod; ++t) {
            const std::vector<Event> tickEvents = game.tick();
            if (events)
                events->insert(events->end(), tickEvents.begin(), tickEvents.end());
        }
    }
    game.releaseDirection(direction);
}

int count(const std::vector<Event> &events, Event::Type type) {
    return int(std::count_if(events.begin(), events.end(), [type](const Event &e) { return e.type == type; }));
}

const Event *find(const std::vector<Event> &events, Event::Type type) {
    for (const Event &e : events) {
        if (e.type == type)
            return &e;
    }
    return nullptr;
}

void skipIntro(Game &game) {
    for (int i = 0; i < Game::kLevelIntroTicks; ++i)
        game.tick();
}

void fence(Game &game, QPoint ball) {
    Field &field = game.mutableField();
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx != 0 || dy != 0)
                field.set({ball.x() + dx, ball.y() + dy}, Cell::Claimed);
        }
    }
}

Game quietGame(int wallX) {
    Game game(Difficulty::Normal, kSeed);
    game.placeChasers({});
    Field &field = game.mutableField();
    for (int y = Field::kBorder; y < field.height() - Field::kBorder; ++y)
        field.set({wallX, y}, Cell::Claimed);
    game.placeBalls({{{wallX + 2, Field::kBorder}, {1, 1}}});
    game.placePlayer({field.width() / 2, field.height() - 1});
    skipIntro(game);
    return game;
}

std::vector<Event> cutColumn(Game &game) {
    std::vector<Event> events;
    walk(game, Direction::Up, game.field().height() - 1, &events);
    return events;
}

}  // namespace Scenario
