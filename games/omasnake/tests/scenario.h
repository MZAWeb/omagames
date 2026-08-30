#pragma once

#include <deque>
#include <vector>

#include "game.h"

// Scenarios and drivers the engine suites share: hand-placed snakes, whole
// moves rather than ticks, and the paths a scripted run walks.
namespace Scenario {

constexpr quint32 kSeed = 20260830u;
// Somewhere no scripted snake ever walks, so a scenario's food stays put.
constexpr QPoint kParked {1, 1};

void skipReady(Game &game);
// One whole move, and everything it reported.
std::vector<Event> stepMove(Game &game);
bool has(const std::vector<Event> &events, Event::Type type);
Event find(const std::vector<Event> &events, Event::Type type);
// A body of `length` cells laid out in a row, head first, heading right.
std::deque<QPoint> row(QPoint head, int length);
// A Hamiltonian path over the whole board: every row swept in the opposite
// direction to the one above it, so consecutive cells are always neighbours.
std::vector<QPoint> serpentine();
// The first `count` cells of a path as a snake, head on the last of them.
std::deque<QPoint> bodyAlong(const std::vector<QPoint> &path, int count);
Direction headingFrom(QPoint from, QPoint to);

}  // namespace Scenario
