#include "spawn.h"

#include <algorithm>

namespace {

// Balls spawn away from the bottom edge, where the marker starts.
constexpr int kSpawnClearance = 10;

}  // namespace

namespace Spawn {

std::vector<Ball> balls(const Field &field, int count, QRandomGenerator &rng) {
    std::vector<Ball> spawned;
    const int left = Field::kBorder;
    const int right = field.width() - Field::kBorder;
    const int top = Field::kBorder;
    const int bottom = field.height() - Field::kBorder - kSpawnClearance;
    while (int(spawned.size()) < count) {
        const QPoint pos {int(rng.bounded(left, right)), int(rng.bounded(top, bottom))};
        const bool taken = std::any_of(spawned.begin(), spawned.end(), [pos](const Ball &b) { return b.pos == pos; });
        if (taken)
            continue;
        const QPoint dir {rng.bounded(2) ? 1 : -1, rng.bounded(2) ? 1 : -1};
        spawned.push_back({pos, dir});
    }
    return spawned;
}

// Spread along the top edge, the far side of the frame from where the marker
// starts, all of them crawling clockwise.
std::vector<Chaser> chasers(const Field &field, int count, QRandomGenerator &rng) {
    std::vector<Chaser> spawned;
    for (int i = 0; i < count; ++i) {
        const QPoint pos {int(rng.bounded(Field::kBorder, field.width() - Field::kBorder)), 0};
        spawned.push_back({pos, {1, 0}});
    }
    return spawned;
}

std::vector<int> chaserDistances(const Field &field, const std::vector<Chaser> &chasers) {
    std::vector<int> distance(size_t(field.cellCount()), kUnreached);
    std::vector<int> queue;
    for (const Chaser &c : chasers) {
        if (!field.contains(c.pos) || field.at(c.pos) != Cell::Claimed)
            continue;
        const int start = field.index(c.pos);
        if (distance[size_t(start)] != kUnreached)
            continue;
        distance[size_t(start)] = 0;
        queue.push_back(start);
    }
    for (size_t head = 0; head < queue.size(); ++head) {
        const QPoint p = field.point(queue[head]);
        const int next = distance[size_t(queue[head])] + 1;
        for (QPoint step : {QPoint(1, 0), QPoint(0, 1), QPoint(-1, 0), QPoint(0, -1)}) {
            const QPoint n = p + step;
            if (!field.contains(n) || field.at(n) != Cell::Claimed)
                continue;
            const int index = field.index(n);
            if (distance[size_t(index)] != kUnreached)
                continue;
            distance[size_t(index)] = next;
            queue.push_back(index);
        }
    }
    return distance;
}

QPoint playerStart(const Field &field, const std::vector<Chaser> &chasers) {
    // Centre first on ties.
    const int y = field.height() - 1;
    const std::vector<int> distance = chaserDistances(field, chasers);
    const int centre = field.width() / 2;
    QPoint best {centre, y};
    int bestDistance = -1;
    for (int offset = 0; offset < field.width(); ++offset) {
        for (int x : {centre - offset, centre + offset}) {
            if (x < 0 || x >= field.width())
                continue;
            const int nearest = distance[size_t(field.index({x, y}))];
            if (nearest > bestDistance) {
                bestDistance = nearest;
                best = {x, y};
            }
        }
    }
    return best;
}

}  // namespace Spawn
