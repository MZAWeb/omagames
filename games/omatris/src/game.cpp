#include "game.h"

namespace {

// The four corners of a T's box, and which two of them the T points between
// in each rotation state: three filled corners make a spin, and both front
// corners filled make it a full one.
const QPoint kCorners[4] = {{0, 0}, {2, 0}, {0, 2}, {2, 2}};
const int kFrontCorners[Piece::kStates][2] = {{0, 1}, {1, 3}, {2, 3}, {0, 2}};

QPoint middleOf(const PieceCells &cells) {
    QPoint sum;
    for (QPoint cell : cells)
        sum += cell;
    return sum / int(cells.size());
}

}  // namespace

Game::Game(Mode mode, quint32 seed)
    : m_mode(mode), m_params(Rules::params(mode)), m_bag(seed) {
    // Its own stream, so the mess and the pieces dealt to fix it are not the
    // same shuffle read twice.
    if (m_params.dealtStack)
        m_stack.emplace(m_board, ~seed);
    std::vector<Event> ignored;
    spawnNext(ignored);
}

Placement Game::ghost() const {
    Placement landing = m_piece;
    while (m_board.fits(landing.moved(0, 1)))
        landing = landing.moved(0, 1);
    return landing;
}

std::vector<PieceType> Game::nextQueue() const {
    std::vector<PieceType> queue;
    queue.reserve(size_t(Rules::kNextQueue));
    for (int i = 0; i < Rules::kNextQueue; ++i)
        queue.push_back(m_bag.peek(i));
    return queue;
}

void Game::placePiece(const Placement &placement) {
    m_piece = placement;
    m_hasPiece = true;
    m_spin = Spin::None;
    m_gravity = 0;
    m_lockDelay.start(placement.origin.y());
}

const std::vector<int> &Game::dealtStack() const {
    static const std::vector<int> none;
    return m_stack ? m_stack->rows() : none;
}

bool Game::goalReached() const {
    // Rows that are flashing are as good as gone, so the finish agrees with
    // the lock that cleared them.
    if (m_stack)
        return dealtRowsLeft() == 0;
    return m_params.lineGoal > 0 && lines() >= m_params.lineGoal;
}

bool Game::grounded() const {
    return m_hasPiece && !m_board.fits(m_piece.moved(0, 1));
}

bool Game::shift(int dx) {
    if (!playable() || !m_hasPiece)
        return false;
    const Placement moved = m_piece.moved(dx, 0);
    if (!m_board.fits(moved))
        return false;
    m_piece = moved;
    m_spin = Spin::None;
    m_lockDelay.moved(m_piece.origin.y(), grounded());
    return true;
}

bool Game::slide(int dx) {
    if (!playable() || !m_hasPiece)
        return false;
    Placement moved = m_piece;
    while (m_board.fits(moved.moved(dx, 0)))
        moved = moved.moved(dx, 0);
    if (moved.origin == m_piece.origin)
        return false;
    m_piece = moved;
    m_spin = Spin::None;
    m_lockDelay.moved(m_piece.origin.y(), grounded());
    return true;
}

bool Game::rotate(int quarters) {
    if (!playable() || !m_hasPiece)
        return false;
    const int from = m_piece.rotation;
    const int to = Piece::turn(from, quarters);
    if (to == from)
        return false;
    const std::array<QPoint, Piece::kMaxKicks> &kicks = Piece::kicks(m_piece.type, from, to);
    for (int i = 0; i < Piece::kMaxKicks; ++i) {
        Placement candidate = m_piece;
        candidate.rotation = to;
        candidate.origin += kicks[size_t(i)];
        if (!m_board.fits(candidate))
            continue;
        m_piece = candidate;
        m_spin = m_piece.type == PieceType::T ? detectSpin(i) : Spin::None;
        m_lockDelay.moved(m_piece.origin.y(), grounded());
        return true;
    }
    return false;
}

Spin Game::detectSpin(int kickIndex) const {
    bool corner[4];
    int filled = 0;
    for (int i = 0; i < 4; ++i) {
        corner[i] = m_board.blocked(m_piece.origin + kCorners[i]);
        filled += corner[i] ? 1 : 0;
    }
    if (filled < 3)
        return Spin::None;
    const int *front = kFrontCorners[m_piece.rotation];
    if (corner[front[0]] && corner[front[1]])
        return Spin::Full;
    // The last kick of the table is the one only a proper spin can reach, so
    // it promotes what the corners alone would call a mini.
    return kickIndex == Piece::kSpinKick ? Spin::Full : Spin::Mini;
}

qint64 Game::gravityThisTick() const {
    const qint64 base = Rules::gravityPerTick(gravityLevel());
    if (!m_softDrop)
        return base;
    // Instant is the whole well in one tick; the loop below stops at the floor.
    if (m_softDropFactor <= 0)
        return qint64(Rules::kGravityUnit) * Board::kHeight;
    return base * m_softDropFactor;
}

void Game::applyGravity() {
    m_gravity += gravityThisTick();
    int dropped = 0;
    while (m_gravity >= Rules::kGravityUnit) {
        m_gravity -= Rules::kGravityUnit;
        if (!m_board.fits(m_piece.moved(0, 1))) {
            m_gravity = 0;
            break;
        }
        m_piece = m_piece.moved(0, 1);
        ++dropped;
    }
    if (dropped == 0)
        return;
    if (m_softDrop)
        m_scoring.addSoftDrop(dropped);
    m_spin = Spin::None;
    m_lockDelay.fell(m_piece.origin.y());
}

std::vector<Event> Game::hardDrop() {
    std::vector<Event> events;
    if (!playable() || !m_hasPiece)
        return events;
    int cells = 0;
    while (m_board.fits(m_piece.moved(0, 1))) {
        m_piece = m_piece.moved(0, 1);
        ++cells;
    }
    if (cells > 0) {
        m_scoring.addHardDrop(cells);
        m_spin = Spin::None;
    }
    lockPiece(events);
    return events;
}

std::vector<Event> Game::hold() {
    std::vector<Event> events;
    if (!playable() || !m_hasPiece || m_holdUsed)
        return events;
    const PieceType swapped = m_hold;
    m_hold = m_piece.type;
    m_holdUsed = true;
    events.push_back(Event {Event::Held});
    spawnPiece(swapped == PieceType::None ? m_bag.take() : swapped, events);
    return events;
}

std::vector<Event> Game::tick() {
    std::vector<Event> events;
    if (!playable())
        return events;
    ++m_ticks;
    if (!m_clearingRows.empty()) {
        if (--m_clearTicks <= 0)
            finishClear(events);
        return events;
    }
    if (!m_hasPiece)
        return events;
    applyGravity();
    if (grounded() && m_lockDelay.rest())
        lockPiece(events);
    return events;
}

void Game::lockPiece(std::vector<Event> &events) {
    const PieceCells cells = m_piece.cells();
    const Spin spin = m_piece.type == PieceType::T ? m_spin : Spin::None;
    Event locked {Event::Locked};
    locked.at = middleOf(cells);
    bool lockedOut = true;
    for (QPoint cell : cells) {
        if (Board::inside(cell))
            locked.cells.push_back(Board::index(cell));
        if (Board::visible(cell))
            lockedOut = false;
    }
    m_board.lock(m_piece);
    m_hasPiece = false;
    // Lock out: the whole piece came to rest above the visible field.
    if (lockedOut) {
        events.push_back(locked);
        topOut(events);
        return;
    }

    const std::vector<int> rows = m_board.fullRows();
    const int before = level();
    locked.clear = m_scoring.award(int(rows.size()), spin);
    events.push_back(locked);
    if (rows.empty()) {
        spawnNext(events);
        return;
    }
    Event cleared {Event::LinesCleared};
    cleared.rows = rows;
    cleared.clear = locked.clear;
    cleared.at = locked.at;
    events.push_back(cleared);
    if (level() != before) {
        Event levelUp {Event::LevelUp};
        levelUp.level = level();
        events.push_back(levelUp);
    }
    m_clearingRows = rows;
    m_clearTicks = Rules::kClearDelayTicks;
    if (goalReached())
        finishClear(events);
}

void Game::finishClear(std::vector<Event> &events) {
    if (m_stack)
        m_stack->clear(m_clearingRows);
    m_board.clearRows(m_clearingRows);
    m_clearingRows.clear();
    m_clearTicks = 0;
    if (goalReached()) {
        if (m_stack)
            m_stack->rate(m_board);
        m_phase = Phase::Finished;
        events.push_back(Event {Event::Finished});
        return;
    }
    spawnNext(events);
}

void Game::spawnNext(std::vector<Event> &events) {
    // Rated when the board is at rest: every piece that settles comes
    // through here, after its lines have cleared.
    if (m_stack)
        m_stack->rate(m_board);
    m_holdUsed = false;
    spawnPiece(m_bag.take(), events);
}

void Game::spawnPiece(PieceType type, std::vector<Event> &events) {
    const Placement placement {type, 0, {Piece::spawnColumn(type, Board::kWidth), kSpawnRow}};
    // Block out: the new piece has nowhere to appear.
    if (!m_board.fits(placement)) {
        m_hasPiece = false;
        topOut(events);
        return;
    }
    placePiece(placement);
}

void Game::topOut(std::vector<Event> &events) {
    m_phase = Phase::GameOver;
    events.push_back(Event {Event::TopOut});
}
