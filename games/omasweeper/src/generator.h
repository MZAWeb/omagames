#pragma once

#include <QPoint>
#include <QtGlobal>

#include "board.h"
#include "presets.h"

struct GenerationResult {
    // Mines placed, nothing revealed: the caller opens `firstClick` next.
    Board board;
    // Boards tried, including the one returned.
    int attempts = 0;
    // True when the solver proved the board; false when the attempt cap ran
    // out and the last board was returned anyway ("may require guessing").
    bool guaranteed = false;
};

// Random boards that logic alone can finish. Each attempt places mines from
// a seed derived from (`seed`, attempt) with the 3×3 around `firstClick`
// kept clear, then asks the Solver; the first board it wins is returned.
namespace Generator {

constexpr int kMaxAttempts = 400;

quint32 attemptSeed(quint32 seed, int attempt);

GenerationResult generate(int width, int height, int mines, QPoint firstClick, quint32 seed,
                          int maxAttempts = kMaxAttempts);
GenerationResult generate(const PresetSpec &preset, QPoint firstClick, quint32 seed,
                          int maxAttempts = kMaxAttempts);

}  // namespace Generator
