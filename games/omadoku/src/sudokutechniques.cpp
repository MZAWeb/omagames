#include "sudokutechniques.h"

// The techniques that live inside one unit. The
// fish and wings, which reach across the grid, are in sudokufish.cpp.
namespace SudokuGrader {
namespace {

// Where `digit` can still go inside `unit`, as a mask over the unit's slots.
Mask spotsIn(const CandidateGrid &grid, const Unit &unit, int digit) {
    Mask spots = 0;
    for (int slot = 0; slot < 9; ++slot) {
        if (grid.has(unit[size_t(slot)], digit))
            spots |= Mask(1u << slot);
    }
    return spots;
}

int soleSlot(Mask spots) { return soleDigit(spots) - 1; }

// A hidden single over the given range of units (see units() for the layout).
bool hiddenSingleIn(CandidateGrid &grid, int firstUnit, int lastUnit) {
    for (int u = firstUnit; u <= lastUnit; ++u) {
        const Unit &unit = units()[size_t(u)];
        for (int d = 1; d <= 9; ++d) {
            const Mask spots = spotsIn(grid, unit, d);
            if (popCount(spots) == 1) {
                grid.place(unit[size_t(soleSlot(spots))], d);
                return true;
            }
        }
    }
    return false;
}

// Digits `keep` are confined to the `cells` slots of `unit`, so those cells
// hold nothing else.
bool eliminateOthersFromSlots(CandidateGrid &grid, const Unit &unit, Mask cells, Mask keep) {
    bool changed = false;
    for (int slot = 0; slot < 9; ++slot) {
        if (cells & Mask(1u << slot))
            changed |= grid.eliminate(unit[size_t(slot)], Mask(kAllDigits & ~keep));
    }
    return changed;
}

// Removes `digits` from every cell of `unit` outside the `keep` slots.
bool eliminateFromUnit(CandidateGrid &grid, const Unit &unit, Mask digits, Mask keep) {
    bool changed = false;
    for (int slot = 0; slot < 9; ++slot) {
        if (!(keep & Mask(1u << slot)))
            changed |= grid.eliminate(unit[size_t(slot)], digits);
    }
    return changed;
}

}  // namespace

QString techniqueId(Technique technique) {
    switch (technique) {
    case Technique::LastDigit:
        return QStringLiteral("last-digit");
    case Technique::HiddenSingleBox:
        return QStringLiteral("hidden-single-box");
    case Technique::NakedSingle:
        return QStringLiteral("naked-single");
    case Technique::HiddenSingleLine:
        return QStringLiteral("hidden-single-line");
    case Technique::NakedPair:
        return QStringLiteral("naked-pair");
    case Technique::HiddenPair:
        return QStringLiteral("hidden-pair");
    case Technique::NakedTriple:
        return QStringLiteral("naked-triple");
    case Technique::HiddenTriple:
        return QStringLiteral("hidden-triple");
    case Technique::XWing:
        return QStringLiteral("x-wing");
    case Technique::Swordfish:
        return QStringLiteral("swordfish");
    case Technique::XYWing:
        break;
    }
    return QStringLiteral("xy-wing");
}

bool techniqueFromId(const QString &id, Technique *technique) {
    for (int t = 0; t < kTechniqueCount; ++t) {
        if (techniqueId(Technique(t)) == id) {
            *technique = Technique(t);
            return true;
        }
    }
    return false;
}

// The last empty cell of a row, column or box: Sudoku Explainer's "last
// value", the one placement that needs no reasoning at all.
bool lastDigit(CandidateGrid &grid) {
    for (const Unit &unit : units()) {
        int empty = -1;
        int count = 0;
        for (int index : unit) {
            if (grid.value(index) == 0) {
                empty = index;
                ++count;
            }
        }
        if (count == 1) {
            grid.place(empty, soleDigit(grid.candidates(empty)));
            return true;
        }
    }
    return false;
}

bool hiddenSingleBox(CandidateGrid &grid) {
    return hiddenSingleIn(grid, 18, kUnits - 1);
}

bool hiddenSingleLine(CandidateGrid &grid) {
    return hiddenSingleIn(grid, 0, 17);
}

bool nakedSingle(CandidateGrid &grid) {
    for (int i = 0; i < Sudoku::kCells; ++i) {
        if (grid.value(i) != 0)
            continue;
        if (const int digit = soleDigit(grid.candidates(i))) {
            grid.place(i, digit);
            return true;
        }
    }
    return false;
}

bool nakedPair(CandidateGrid &grid) {
    for (const Unit &unit : units()) {
        for (int a = 0; a < 9; ++a) {
            const Mask pair = grid.candidates(unit[size_t(a)]);
            if (popCount(pair) != 2)
                continue;
            for (int b = a + 1; b < 9; ++b) {
                if (grid.candidates(unit[size_t(b)]) != pair)
                    continue;
                if (eliminateFromUnit(grid, unit, pair, Mask((1u << a) | (1u << b))))
                    return true;
            }
        }
    }
    return false;
}

bool hiddenPair(CandidateGrid &grid) {
    for (const Unit &unit : units()) {
        std::array<Mask, 10> spots {};
        for (int d = 1; d <= 9; ++d)
            spots[size_t(d)] = spotsIn(grid, unit, d);
        for (int d1 = 1; d1 <= 9; ++d1) {
            if (popCount(spots[size_t(d1)]) != 2)
                continue;
            for (int d2 = d1 + 1; d2 <= 9; ++d2) {
                if (spots[size_t(d2)] != spots[size_t(d1)])
                    continue;
                if (eliminateOthersFromSlots(grid, unit, spots[size_t(d1)], Mask(bitOf(d1) | bitOf(d2))))
                    return true;
            }
        }
    }
    return false;
}

bool nakedTriple(CandidateGrid &grid) {
    for (const Unit &unit : units()) {
        std::array<Mask, 9> cands {};
        for (int slot = 0; slot < 9; ++slot) {
            const Mask mask = grid.candidates(unit[size_t(slot)]);
            // Only cells that can be part of a triple: two or three candidates.
            cands[size_t(slot)] = (popCount(mask) == 2 || popCount(mask) == 3) ? mask : 0;
        }
        for (int a = 0; a < 9; ++a) {
            if (!cands[size_t(a)])
                continue;
            for (int b = a + 1; b < 9; ++b) {
                if (!cands[size_t(b)])
                    continue;
                for (int c = b + 1; c < 9; ++c) {
                    if (!cands[size_t(c)])
                        continue;
                    const Mask triple = Mask(cands[size_t(a)] | cands[size_t(b)] | cands[size_t(c)]);
                    if (popCount(triple) != 3)
                        continue;
                    if (eliminateFromUnit(grid, unit, triple, Mask((1u << a) | (1u << b) | (1u << c))))
                        return true;
                }
            }
        }
    }
    return false;
}

bool hiddenTriple(CandidateGrid &grid) {
    for (const Unit &unit : units()) {
        std::array<Mask, 10> spots {};
        for (int d = 1; d <= 9; ++d) {
            const Mask mask = spotsIn(grid, unit, d);
            // Only digits that can be part of a triple: two or three spots.
            spots[size_t(d)] = (popCount(mask) == 2 || popCount(mask) == 3) ? mask : 0;
        }
        for (int d1 = 1; d1 <= 9; ++d1) {
            if (!spots[size_t(d1)])
                continue;
            for (int d2 = d1 + 1; d2 <= 9; ++d2) {
                if (!spots[size_t(d2)])
                    continue;
                for (int d3 = d2 + 1; d3 <= 9; ++d3) {
                    if (!spots[size_t(d3)])
                        continue;
                    const Mask cells = Mask(spots[size_t(d1)] | spots[size_t(d2)] | spots[size_t(d3)]);
                    if (popCount(cells) != 3)
                        continue;
                    const Mask keep = Mask(bitOf(d1) | bitOf(d2) | bitOf(d3));
                    if (eliminateOthersFromSlots(grid, unit, cells, keep))
                        return true;
                }
            }
        }
    }
    return false;
}

bool apply(Technique technique, CandidateGrid &grid) {
    switch (technique) {
    case Technique::LastDigit:
        return lastDigit(grid);
    case Technique::HiddenSingleBox:
        return hiddenSingleBox(grid);
    case Technique::NakedSingle:
        return nakedSingle(grid);
    case Technique::HiddenSingleLine:
        return hiddenSingleLine(grid);
    case Technique::NakedPair:
        return nakedPair(grid);
    case Technique::HiddenPair:
        return hiddenPair(grid);
    case Technique::NakedTriple:
        return nakedTriple(grid);
    case Technique::HiddenTriple:
        return hiddenTriple(grid);
    case Technique::XWing:
        return xWing(grid);
    case Technique::Swordfish:
        return swordfish(grid);
    case Technique::XYWing:
        break;
    }
    return xyWing(grid);
}

}
