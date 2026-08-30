#include "entities.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

#include "field.h"

namespace {

bool blocksBall(const Field &field, QPoint p) {
    return !field.contains(p) || field.at(p) == Cell::Claimed;
}

// Screen coordinates: y grows downward, so this turns a heading to its right.
QPoint clockwise(QPoint dir) {
    return {-dir.y(), dir.x()};
}

const QPoint kSteps[] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};

int headingIndex(QPoint dir) {
    for (int i = 0; i < 4; ++i) {
        if (kSteps[i] == dir)
            return i;
    }
    return 0;
}

// What a breadth-first walk over claimed ground found: the cells it reached,
// nearest first, the cell each of them was reached from, and the cell it
// stopped at, if any.
struct GroundReach {
    std::vector<int> order;
    std::vector<int> from;
    int destination = -1;
};

// Breadth-first over claimed ground from `start`, stopping at the first cell
// `stop` accepts. The fixed neighbour order keeps every walk deterministic,
// and `order` running nearest-first makes its last edge cell a farthest one.
template <typename Stop>
GroundReach reachOverGround(const Field &field, QPoint start, Stop stop) {
    GroundReach reach;
    reach.from.assign(size_t(field.cellCount()), -1);
    const int first = field.index(start);
    reach.from[size_t(first)] = first;
    reach.order.push_back(first);
    for (size_t head = 0; head < reach.order.size(); ++head) {
        const int current = reach.order[head];
        const QPoint p = field.point(current);
        if (current != first && stop(p)) {
            reach.destination = current;
            break;
        }
        for (QPoint step : kSteps) {
            const QPoint n = p + step;
            if (!field.contains(n) || field.at(n) != Cell::Claimed)
                continue;
            const int next = field.index(n);
            if (reach.from[size_t(next)] >= 0)
                continue;
            reach.from[size_t(next)] = current;
            reach.order.push_back(next);
        }
    }
    return reach;
}

// The first cell of the walk from `start` to `cell`, or `start` itself when
// the walk never reached it.
QPoint firstStep(const Field &field, const GroundReach &reach, QPoint start, int cell) {
    const int first = field.index(start);
    if (cell == first || reach.from[size_t(cell)] < 0)
        return start;
    int step = cell;
    while (reach.from[size_t(step)] != first)
        step = reach.from[size_t(step)];
    return field.point(step);
}

// The stretches of sea a cell looks out on — eight-connected, the same
// neighbourhood that makes a claimed cell an edge cell in the first place.
std::vector<int> regionsAt(const Field &field, const std::vector<int> &region, QPoint p) {
    std::vector<int> ids;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            const QPoint n {p.x() + dx, p.y() + dy};
            if (!field.contains(n))
                continue;
            const int id = region[size_t(field.index(n))];
            if (id != Field::kNoRegion && std::find(ids.begin(), ids.end(), id) == ids.end())
                ids.push_back(id);
        }
    }
    return ids;
}

}  // namespace

QPoint delta(Direction direction) {
    switch (direction) {
    case Direction::Up:
        return {0, -1};
    case Direction::Down:
        return {0, 1};
    case Direction::Left:
        return {-1, 0};
    case Direction::Right:
        return {1, 0};
    case Direction::None:
        break;
    }
    return {0, 0};
}

Direction opposite(Direction direction) {
    switch (direction) {
    case Direction::Up:
        return Direction::Down;
    case Direction::Down:
        return Direction::Up;
    case Direction::Left:
        return Direction::Right;
    case Direction::Right:
        return Direction::Left;
    case Direction::None:
        break;
    }
    return Direction::None;
}

// Reflect Xonix-style: whichever axis is blocked flips, and a blocker sitting
// exactly on the corner flips both. If the way is still shut afterwards the
// ball stays put for this step.
void Ball::step(const Field &field) {
    const bool wallX = blocksBall(field, {pos.x() + dir.x(), pos.y()});
    const bool wallY = blocksBall(field, {pos.x(), pos.y() + dir.y()});
    if (wallX)
        dir.setX(-dir.x());
    if (wallY)
        dir.setY(-dir.y());
    if (!wallX && !wallY && blocksBall(field, pos + dir))
        dir = -dir;
    if (!blocksBall(field, pos + dir))
        pos += dir;
}

void Chaser::step(const Field &field, QRandomGenerator &rng) {
    if (!field.contains(pos) || field.at(pos) != Cell::Claimed)
        return;
    if (field.contains(target)) {
        walkToTarget(field, rng);
        return;
    }
    if (!field.isEdge(pos)) {
        walkBackToTheEdge(field);
        return;
    }
    // A chaser dropped on the field has no heading yet; send it off to the
    // right and let the edge steer it from there.
    if (std::abs(dir.x()) + std::abs(dir.y()) != 1)
        dir = {1, 0};
    if (crawl(field, rng))
        remember(field);
}

void Chaser::forgetTrack() {
    track.clear();
    stale = 0;
}

// One step along the boundary. The ways on are the neighbouring edge cells
// other than the one the chaser came from; where the boundary branches there
// is more than one and the seed picks. Turning back is for dead ends only.
// Returns false when the chaser had nowhere at all to go.
bool Chaser::crawl(const Field &field, QRandomGenerator &rng) {
    QPoint ways[3];
    int count = 0;
    for (QPoint way : {clockwise(dir), dir, -clockwise(dir)}) {
        if (field.isEdge(pos + way))
            ways[count++] = way;
    }
    if (count == 0) {
        if (!field.isEdge(pos - dir))
            return false;
        dir = -dir;
    } else {
        dir = ways[count == 1 ? 0 : int(rng.bounded(count))];
    }
    pos += dir;
    return true;
}

// Loop detection. A (cell, heading) the chaser has already been in means it
// has come full circle, and a long stretch without a new cell means it is
// only pacing one it knows; either way the boundary has nothing left to show.
void Chaser::remember(const Field &field) {
    const int cell = field.index(pos) * 4;
    bool seen = false;
    for (int heading = 0; heading < 4; ++heading)
        seen = seen || track.count(cell + heading) > 0;
    stale = seen ? stale + 1 : 0;
    const bool circuit = !track.insert(cell + headingIndex(dir)).second;
    if (circuit || stale >= kStaleSteps)
        retarget(field);
}

void Chaser::retarget(const Field &field) {
    forgetTrack();
    const QPoint destination = elsewhere(field);
    if (destination != pos)
        target = destination;
}

// Where a chaser that has run out of boundary goes: the nearest edge cell
// opening onto a stretch of sea its own does not touch, or, when the field
// holds a single one, the farthest edge cell it can walk to.
QPoint Chaser::elsewhere(const Field &field) const {
    const std::vector<int> region = field.openRegions();
    const std::vector<int> mine = regionsAt(field, region, pos);
    const auto foreign = [&](QPoint p) {
        if (!field.isEdge(p))
            return false;
        const std::vector<int> ids = regionsAt(field, region, p);
        return std::any_of(ids.begin(), ids.end(), [&mine](int id) {
            return std::find(mine.begin(), mine.end(), id) == mine.end();
        });
    };
    const GroundReach reach = reachOverGround(field, pos, foreign);
    if (reach.destination >= 0)
        return field.point(reach.destination);
    // One region only: the far side of it is as good as it gets.
    for (auto it = reach.order.rbegin(); it != reach.order.rend(); ++it) {
        const QPoint p = field.point(*it);
        if (field.isEdge(p))
            return p;
    }
    return pos;
}

// One cell of the walk across the ground toward the boundary the chaser has
// set its mind on. It arrives with a heading of the seed's choosing, so it
// does not simply crawl the way it came.
void Chaser::walkToTarget(const Field &field, QRandomGenerator &rng) {
    const int goal = field.index(target);
    const GroundReach reach = reachOverGround(field, pos, [goal, &field](QPoint p) { return field.index(p) == goal; });
    const QPoint next = firstStep(field, reach, pos, goal);
    if (next == pos) {
        target = {-1, -1};
        return;
    }
    dir = next - pos;
    pos = next;
    if (pos == target) {
        target = {-1, -1};
        dir = kSteps[rng.bounded(4)];
    }
}

// A claim can bury a chaser deep inside the ground it used to hug. Walk it
// back out along the shortest path over claimed cells.
void Chaser::walkBackToTheEdge(const Field &field) {
    const GroundReach reach = reachOverGround(field, pos, [&field](QPoint p) { return field.isEdge(p); });
    if (reach.destination < 0)
        return;
    const QPoint next = firstStep(field, reach, pos, reach.destination);
    if (next == pos)
        return;
    dir = next - pos;
    pos = next;
}
