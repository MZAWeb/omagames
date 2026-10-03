#pragma once

#include <vector>

#include "observationlayout.h"
#include "placements.h"

// What an agent sees of a game of Omatris, as observation tensors. Pieces
// are numbered PieceType + 1, so 0 is always "nothing" (an empty cell, no
// held piece). Only what a player sees: the board, the falling piece, the
// next three and the hold box; the bag behind them stays hidden.
class OmatrisObservation {
public:
    // `candidates` > 0 adds the landings tensors the placement action space
    // picks from: one row per landing, and the board each would leave.
    explicit OmatrisObservation(int candidates);

    const OmaGames::ObservationLayout &layout() const { return m_layout; }
    void write(std::byte *buffer, const Game &game, const std::vector<Landing> &landings) const;

private:
    OmaGames::ObservationLayout m_layout;
    int m_candidates;
    int m_board = 0;
    int m_piece = 0;
    int m_queue = 0;
    int m_hold = 0;
    int m_stats = 0;
    int m_landingRows = 0;
    int m_afterstates = 0;
};
