#pragma once

#include <QPoint>
#include <QRandomGenerator>
#include <limits>
#include <vector>

#include "entities.h"
#include "field.h"

// Where a level's movers and the marker start. Pure functions of the field
// and the seeded generator, so a level lays itself out identically every run.
namespace Spawn {

// No chaser can walk to this cell at all.
constexpr int kUnreached = std::numeric_limits<int>::max();

std::vector<Ball> balls(const Field &field, int count, QRandomGenerator &rng);
std::vector<Chaser> chasers(const Field &field, int count, QRandomGenerator &rng);
// How far each claimed cell is from the nearest chaser, counted in the steps
// a chaser would actually take: chasers never leave the ground, so a cell
// across the sea is not near at all. Unreached cells come back as kUnreached.
std::vector<int> chaserDistances(const Field &field, const std::vector<Chaser> &chasers);
// The bottom-edge cell the nearest chaser has the longest crawl to.
QPoint playerStart(const Field &field, const std::vector<Chaser> &chasers);

}  // namespace Spawn
