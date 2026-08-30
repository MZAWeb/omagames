#include "generator.h"

#include "solver.h"

// Golden-ratio stride: consecutive attempts land far apart in seed space.
quint32 Generator::attemptSeed(quint32 seed, int attempt) { return seed + quint32(attempt) * 0x9E3779B9u; }

GenerationResult Generator::generate(int width, int height, int mines, QPoint firstClick, quint32 seed,
                                     int maxAttempts) {
    GenerationResult result{Board(width, height, mines, seed), 0, false};
    for (int attempt = 0; attempt < std::max(1, maxAttempts); ++attempt) {
        Board board(width, height, mines, attemptSeed(seed, attempt));
        board.placeMines(board.seed(), firstClick);
        result.attempts = attempt + 1;
        result.board = board;
        if (Solver::isSolvableWithoutGuessing(board, firstClick)) {
            result.guaranteed = true;
            break;
        }
    }
    return result;
}

GenerationResult Generator::generate(const PresetSpec &preset, QPoint firstClick, quint32 seed, int maxAttempts) {
    return generate(preset.width, preset.height, preset.mines, firstClick, seed, maxAttempts);
}
