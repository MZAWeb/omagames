# Omasweeper

Minesweeper for Omarchy, with one promise the original never made: **every
board can be finished by logic alone**. No 50/50s, no guessing, ever. Your
first click always opens a zero region, and from there each mine can be
proved.

## Rules

- Three presets: **Beginner** 9×9 with 10 mines, **Intermediate** 16×16 with
  40, **Expert** 30×16 with 99.
- Reveal a cell. A mine ends the game; a number tells how many of the eight
  cells around it are mines; a zero opens every cell around it, rippling
  outwards until numbers fence it in.
- Flag a hidden cell you have proved to be a mine; flag it again to clear
  the flag. Flags are for you: the game does not need them to be won.
- **Chord** on a revealed number that already has as many flags around it as
  its value: every other hidden neighbour opens at once. A wrong flag means
  one of them is a mine, and that is a loss.
- The game is won when every cell without a mine is revealed.
- The mine counter shows mines minus flags, and goes negative if you
  over-flag. The clock runs from your first reveal to the win or the loss.

## The no-guess promise

Mines are placed after the first click, keeping the 3×3 around it clear so
the opening is always a zero. The board is then played by a built-in logical
solver that only sees what you see: it uses satisfied and saturated numbers,
subset reasoning between overlapping numbers (the 1-2-1 family), and an
exact enumeration of every consistent mine layout along the frontier,
cross-checked against the mines still unaccounted for. If the solver gets
stuck, the layout is thrown away and another is drawn from the next seed,
until it finds one it can finish. Beginner and Intermediate boards need one
or two tries; Expert about six, still well under a hundredth of a second.

The solver has a bounded search budget and the generator a bounded number
of tries. Neither bound has been hit on any seed tested, but if either ever
is, the game says so ("may require guessing") rather than pretending.

## Keys

_To be written with the UI._

## Install

_To be written with the packaging._
