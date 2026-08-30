#include "board.h"

#include <QJsonArray>
#include <QRandomGenerator>
#include <QString>
#include <algorithm>
#include <deque>

namespace {

int clampSize(int size) { return std::clamp(size, Board::kMinSize, Board::kMaxSize); }

// At least the clicked cell itself stays free.
int clampMines(int mines, int cells) { return std::clamp(mines, 0, cells - 1); }

QChar stateChar(const Cell &cell) {
    switch (cell.state) {
    case CellState::Hidden:
        return cell.mine ? QLatin1Char('*') : QLatin1Char('.');
    case CellState::Flagged:
        return cell.mine ? QLatin1Char('F') : QLatin1Char('f');
    case CellState::Revealed:
        return QLatin1Char('r');
    }
    return QLatin1Char('.');
}

}  // namespace

Board::Board(int width, int height, int mines, quint32 seed)
    : m_width(clampSize(width)),
      m_height(clampSize(height)),
      m_mines(clampMines(mines, m_width * m_height)),
      m_seed(seed),
      m_cells(size_t(m_width * m_height)) {}

bool Board::contains(QPoint p) const {
    return p.x() >= 0 && p.y() >= 0 && p.x() < m_width && p.y() < m_height;
}

std::vector<int> Board::neighbours(int index) const {
    std::vector<int> result;
    result.reserve(8);
    const QPoint p = point(index);
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            const QPoint q(p.x() + dx, p.y() + dy);
            if ((dx != 0 || dy != 0) && contains(q))
                result.push_back(this->index(q));
        }
    }
    return result;
}

std::vector<int> Board::mines() const {
    std::vector<int> result;
    for (int i = 0; i < cellCount(); ++i) {
        if (m_cells[size_t(i)].mine)
            result.push_back(i);
    }
    return result;
}

void Board::placeMines(quint32 seed, QPoint firstClick) {
    if (m_minesPlaced)
        return;
    const int click = contains(firstClick) ? index(firstClick) : -1;
    std::vector<int> excluded;
    if (click >= 0) {
        excluded = neighbours(click);
        excluded.push_back(click);
    }
    std::vector<int> eligible;
    for (int i = 0; i < cellCount(); ++i) {
        if (std::find(excluded.begin(), excluded.end(), i) == excluded.end())
            eligible.push_back(i);
    }
    if (int(eligible.size()) < m_mines) {
        eligible.clear();
        for (int i = 0; i < cellCount(); ++i) {
            if (i != click)
                eligible.push_back(i);
        }
    }
    // Partial Fisher–Yates: the first m_mines entries end up uniformly chosen.
    QRandomGenerator rng(seed);
    for (int i = 0; i < m_mines; ++i)
        std::swap(eligible[size_t(i)], eligible[size_t(i) + size_t(rng.bounded(int(eligible.size()) - i))]);
    eligible.resize(size_t(m_mines));
    placeMines(eligible);
}

void Board::placeMines(const std::vector<int> &mines) {
    if (m_minesPlaced)
        return;
    for (Cell &cell : m_cells)
        cell.mine = false;
    for (int i : mines) {
        if (i >= 0 && i < cellCount())
            m_cells[size_t(i)].mine = true;
    }
    m_mines = int(std::count_if(m_cells.begin(), m_cells.end(), [](const Cell &c) { return c.mine; }));
    m_minesPlaced = true;
    computeAdjacent();
}

void Board::computeAdjacent() {
    for (int i = 0; i < cellCount(); ++i) {
        int count = 0;
        for (int n : neighbours(i))
            count += m_cells[size_t(n)].mine ? 1 : 0;
        m_cells[size_t(i)].adjacent = quint8(count);
    }
}

MoveResult Board::reveal(QPoint p) {
    MoveResult result;
    result.status = m_status;
    if (m_status == Status::Won || m_status == Status::Lost || !contains(p))
        return result;
    const int i = index(p);
    if (m_cells[size_t(i)].state != CellState::Hidden)
        return result;
    if (!m_minesPlaced)
        placeMines(m_seed, p);
    m_status = Status::Playing;
    if (m_cells[size_t(i)].mine) {
        lose(i, result);
        return result;
    }
    cascade(i, result);
    finish(result);
    return result;
}

MoveResult Board::toggleFlag(QPoint p) {
    MoveResult result;
    result.status = m_status;
    if (m_status == Status::Won || m_status == Status::Lost || !contains(p))
        return result;
    Cell &cell = m_cells[size_t(index(p))];
    if (cell.state == CellState::Hidden) {
        cell.state = CellState::Flagged;
        ++m_flags;
    } else if (cell.state == CellState::Flagged) {
        cell.state = CellState::Hidden;
        --m_flags;
    } else {
        return result;
    }
    result.toggled = index(p);
    return result;
}

MoveResult Board::chord(QPoint p) {
    MoveResult result;
    result.status = m_status;
    if (m_status != Status::Playing || !contains(p))
        return result;
    const Cell &cell = m_cells[size_t(index(p))];
    if (cell.state != CellState::Revealed || cell.adjacent == 0)
        return result;
    const std::vector<int> around = neighbours(index(p));
    int flags = 0;
    for (int n : around)
        flags += m_cells[size_t(n)].state == CellState::Flagged ? 1 : 0;
    if (flags != cell.adjacent)
        return result;
    for (int n : around) {
        if (m_cells[size_t(n)].state != CellState::Hidden)
            continue;
        if (m_cells[size_t(n)].mine) {
            lose(n, result);
            return result;
        }
        cascade(n, result);
    }
    finish(result);
    return result;
}

// Breadth-first so the result ripples outwards from `start`.
void Board::cascade(int start, MoveResult &result) {
    std::deque<int> queue;
    queue.push_back(start);
    m_cells[size_t(start)].state = CellState::Revealed;
    while (!queue.empty()) {
        const int i = queue.front();
        queue.pop_front();
        ++m_revealed;
        result.revealed.push_back(i);
        if (m_cells[size_t(i)].adjacent != 0)
            continue;
        for (int n : neighbours(i)) {
            Cell &next = m_cells[size_t(n)];
            if (next.state == CellState::Hidden) {
                next.state = CellState::Revealed;
                queue.push_back(n);
            }
        }
    }
}

void Board::lose(int exploded, MoveResult &result) {
    m_status = Status::Lost;
    result.exploded = exploded;
    for (int i = 0; i < cellCount(); ++i) {
        const Cell &cell = m_cells[size_t(i)];
        if (cell.state == CellState::Flagged && !cell.mine)
            result.wrongFlags.push_back(i);
    }
    result.status = m_status;
}

void Board::finish(MoveResult &result) {
    if (m_revealed == safeCount())
        m_status = Status::Won;
    result.status = m_status;
}

QJsonObject Board::toJson() const {
    QString cells;
    cells.reserve(cellCount());
    for (const Cell &cell : m_cells)
        cells.append(stateChar(cell));
    QJsonObject json;
    json[QStringLiteral("width")] = m_width;
    json[QStringLiteral("height")] = m_height;
    json[QStringLiteral("mines")] = m_mines;
    json[QStringLiteral("seed")] = double(m_seed);
    json[QStringLiteral("placed")] = m_minesPlaced;
    json[QStringLiteral("status")] = int(m_status);
    json[QStringLiteral("cells")] = cells;
    return json;
}

std::optional<Board> Board::fromJson(const QJsonObject &json) {
    const int width = json.value(QStringLiteral("width")).toInt();
    const int height = json.value(QStringLiteral("height")).toInt();
    const QString cells = json.value(QStringLiteral("cells")).toString();
    if (width < kMinSize || height < kMinSize || width > kMaxSize || height > kMaxSize
        || cells.size() != width * height)
        return std::nullopt;
    Board board(width, height, json.value(QStringLiteral("mines")).toInt(),
                quint32(json.value(QStringLiteral("seed")).toDouble()));
    if (json.value(QStringLiteral("placed")).toBool()) {
        std::vector<int> mines;
        for (int i = 0; i < cells.size(); ++i) {
            if (cells[i] == QLatin1Char('*') || cells[i] == QLatin1Char('F'))
                mines.push_back(i);
        }
        board.placeMines(mines);
    }
    for (int i = 0; i < cells.size(); ++i) {
        Cell &cell = board.m_cells[size_t(i)];
        if (cells[i] == QLatin1Char('r')) {
            cell.state = CellState::Revealed;
            ++board.m_revealed;
        } else if (cells[i] == QLatin1Char('f') || cells[i] == QLatin1Char('F')) {
            cell.state = CellState::Flagged;
            ++board.m_flags;
        }
    }
    const int status = json.value(QStringLiteral("status")).toInt();
    if (status < int(Status::Ready) || status > int(Status::Lost))
        return std::nullopt;
    board.m_status = Status(status);
    return board;
}
