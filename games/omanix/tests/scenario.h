#pragma once

#include <vector>

#include "game.h"

// Scenarios and drivers the engine suites share: a seeded game with nothing
// in the way, and the ticking it takes to walk the marker around it.
namespace Scenario {

constexpr quint32 kSeed = 20260830u;
constexpr int kInteriorHeight = Field::kDefaultHeight - 2 * Field::kBorder;

// Runs ticks until the player has moved `cells` times (or gives up).
void walk(Game &game, Direction direction, int cells, std::vector<Event> *events = nullptr);
int count(const std::vector<Event> &events, Event::Type type);
const Event *find(const std::vector<Event> &events, Event::Type type);
void skipIntro(Game &game);
// Walls a ball into a one-cell pocket so it stays exactly where a scenario
// put it. Its region is then that single cell, so any claim afterwards takes
// the whole rest of the field.
void fence(Game &game, QPoint ball);
// A game with nothing that could interfere: no chasers, and one ball kept on
// the far side of a claimed wall down column `wallX`. Any cut left of the
// wall closes with no ball on that side, so it claims every interior column
// up to the wall: `(wallX - kBorder) * kInteriorHeight` cells. The marker
// starts bottom-centre.
Game quietGame(int wallX = 40);
// Cuts a full-height column at the marker's x, from the bottom frame up to
// the top.
std::vector<Event> cutColumn(Game &game);

}  // namespace Scenario
