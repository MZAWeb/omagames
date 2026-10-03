#include "placements.h"

#include <algorithm>
#include <unordered_set>

#include "handling.h"

namespace Placements {

namespace {

enum class Move : quint8 { Left, Right, RotateCW, RotateCCW, SonicDrop };
constexpr Move kMoves[] = {Move::Left, Move::Right, Move::RotateCW, Move::RotateCCW, Move::SonicDrop};

void appendCalls(std::vector<Call> &calls, Move move) {
    switch (move) {
    case Move::Left:
        calls.push_back(Call::Left);
        break;
    case Move::Right:
        calls.push_back(Call::Right);
        break;
    case Move::RotateCW:
        calls.push_back(Call::RotateCW);
        break;
    case Move::RotateCCW:
        calls.push_back(Call::RotateCCW);
        break;
    case Move::SonicDrop:
        calls.insert(calls.end(), {Call::SoftDropOn, Call::Tick, Call::SoftDropOff});
        break;
    }
}

// The same calls appendCalls() writes down, made without writing them.
void perform(Game &game, Move move) {
    switch (move) {
    case Move::Left:
        Calls::apply(game, Call::Left);
        break;
    case Move::Right:
        Calls::apply(game, Call::Right);
        break;
    case Move::RotateCW:
        Calls::apply(game, Call::RotateCW);
        break;
    case Move::RotateCCW:
        Calls::apply(game, Call::RotateCCW);
        break;
    case Move::SonicDrop:
        for (Call call : {Call::SoftDropOn, Call::Tick, Call::SoftDropOff})
            Calls::apply(game, call);
        break;
    }
}

bool live(const Game &game) {
    return game.phase() == Phase::Playing && game.hasPiece();
}

bool resting(const Game &game) {
    return game.ghost().origin == game.piece().origin;
}

// A state the search has been in: the piece where it is, and how it would
// spin. Kicks keep the origin within a few cells of the well, which is all
// the room the offsets leave.
int stateKey(const Game &game) {
    const Placement &p = game.piece();
    const int x = p.origin.x() + 4;
    const int y = p.origin.y() + 4;
    Q_ASSERT(x >= 0 && x < 16 && y >= 0 && y < 32);
    return ((p.rotation * 16 + x) * 32 + y) * 3 + int(game.spin());
}
constexpr int kStateKeys = 4 * 16 * 32 * 3;

// A resting place: the cells filled, whichever rotation filled them, and the
// spin. Cell indices are under 256, so four of them pack into 32 bits.
quint64 landingKey(const Game &game) {
    std::array<int, 4> cells {};
    const PieceCells pieceCells = game.piece().cells();
    for (size_t i = 0; i < cells.size(); ++i)
        cells[i] = Board::index(pieceCells[i]);
    std::sort(cells.begin(), cells.end());
    quint64 key = quint64(game.spin());
    for (int cell : cells)
        key = (key << 8) | quint64(cell);
    return key;
}

Landing land(const Game &game, bool hold, std::vector<Call> calls) {
    Landing landing;
    landing.hold = hold;
    landing.placement = game.piece();
    landing.spin = game.spin();
    landing.calls = std::move(calls);
    Game locked = game;
    const std::vector<Event> events = locked.hardDrop();
    for (const Event &event : events) {
        if (event.type == Event::Locked)
            landing.lines = event.clear.lines;
    }
    landing.toppedOut = locked.phase() == Phase::GameOver;
    landing.afterstate = locked.board();
    landing.afterstate.clearRows(landing.afterstate.fullRows());
    return landing;
}

struct Node {
    Game game;
    int parent;
    Move move;
};

std::vector<Call> pathTo(const std::vector<Node> &nodes, int index, const std::vector<Call> &prefix) {
    std::vector<Move> moves;
    for (int i = index; nodes[size_t(i)].parent >= 0; i = nodes[size_t(i)].parent)
        moves.push_back(nodes[size_t(i)].move);
    std::vector<Call> calls = prefix;
    for (auto it = moves.rbegin(); it != moves.rend(); ++it)
        appendCalls(calls, *it);
    return calls;
}

void search(const Game &start, bool hold, const std::vector<Call> &prefix, std::vector<Landing> &landings) {
    std::vector<char> seen(kStateKeys, 0);
    std::unordered_set<quint64> landed;
    std::vector<Node> nodes;
    // Comfortably more states than a piece usually reaches, so the search
    // rarely moves them all to grow.
    nodes.reserve(512);
    nodes.push_back({start, -1, Move::Left});
    seen[size_t(stateKey(start))] = 1;
    for (size_t next = 0; next < nodes.size(); ++next) {
        if (resting(nodes[next].game) && landed.insert(landingKey(nodes[next].game)).second)
            landings.push_back(land(nodes[next].game, hold, pathTo(nodes, int(next), prefix)));
        for (Move move : kMoves) {
            Game moved = nodes[next].game;
            perform(moved, move);
            if (!live(moved))
                continue;
            char &visited = seen[size_t(stateKey(moved))];
            if (visited)
                continue;
            visited = 1;
            nodes.push_back({std::move(moved), int(next), move});
        }
    }
}

}  // namespace

std::vector<Landing> find(const Game &game, bool withHold) {
    Q_ASSERT(game.softDropFactor() == Handling::kInstantSoftDrop);
    std::vector<Landing> landings;
    if (!live(game))
        return landings;
    search(game, false, {}, landings);
    if (withHold && game.holdAvailable()) {
        Game held = game;
        held.hold();
        if (live(held))
            search(held, true, {Call::Hold}, landings);
    }
    return landings;
}

std::optional<Landing> drop(const Game &game, bool hold, int rotation, int column) {
    Q_ASSERT(game.softDropFactor() == Handling::kInstantSoftDrop);
    if (!live(game) || (hold && !game.holdAvailable()))
        return std::nullopt;
    Game moving = game;
    std::vector<Call> calls;
    auto call = [&moving, &calls](Call c) {
        calls.push_back(c);
        Calls::apply(moving, c);
    };
    if (hold) {
        call(Call::Hold);
        if (!live(moving))
            return std::nullopt;
    }
    // Three turns clockwise are one the other way, as a player would do it.
    const int quarters = ((rotation % Piece::kStates) + Piece::kStates) % Piece::kStates;
    for (int i = 0; i < (quarters == 3 ? 1 : quarters); ++i)
        call(quarters == 3 ? Call::RotateCCW : Call::RotateCW);
    if (moving.piece().rotation != quarters)
        return std::nullopt;
    while (leftColumn(moving.piece()) != column) {
        const int before = leftColumn(moving.piece());
        call(before < column ? Call::Right : Call::Left);
        if (leftColumn(moving.piece()) == before)
            return std::nullopt;
    }
    for (Call c : {Call::SoftDropOn, Call::Tick, Call::SoftDropOff})
        call(c);
    if (!live(moving))
        return std::nullopt;
    return land(moving, hold, std::move(calls));
}

int leftColumn(const Placement &placement) {
    const PieceCells cells = placement.cells();
    return std::min_element(cells.begin(), cells.end(), [](QPoint a, QPoint b) { return a.x() < b.x(); })->x();
}

int bottomRow(const Placement &placement) {
    const PieceCells cells = placement.cells();
    return std::max_element(cells.begin(), cells.end(), [](QPoint a, QPoint b) { return a.y() < b.y(); })->y();
}

}  // namespace Placements
