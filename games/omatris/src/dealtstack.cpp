#include "dealtstack.h"

#include <algorithm>

#include "challenge.h"
#include "difficulty.h"

DealtStack::DealtStack(Board &board, quint32 seed)
    : m_rows(Challenge::build(board, seed)), m_dealt(int(m_rows.size())) {
    rate(board);
    m_dealtDifficulty = m_difficulty;
}

int DealtStack::rowsLeft(const std::vector<int> &clearing) const {
    return int(std::count_if(m_rows.begin(), m_rows.end(), [&clearing](int y) {
        return std::find(clearing.begin(), clearing.end(), y) == clearing.end();
    }));
}

void DealtStack::clear(const std::vector<int> &cleared) {
    m_rows = Challenge::afterClear(m_rows, cleared);
}

void DealtStack::rate(const Board &board) {
    m_difficulty = Difficulty::rate(board, m_rows);
}
