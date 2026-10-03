"""What omagym knows about each game beyond its env spec: how to evaluate it
fairly and which numbers to put beside the score when comparing runs.

Every run plays the game's *main test* (the evaluation games compare ranks
by default), then each of its other tests, so one training is tested every
way the game knows. A test is a set of fixed games and what "better" means
on them.
"""

from __future__ import annotations

from dataclasses import dataclass, field


@dataclass(frozen=True)
class Test:
    """One way to test an agent: env settings on top of the run's, the games,
    and the measure `compare --test <name>` ranks by."""

    name: str
    about: str
    env: dict
    episodes: int
    max_steps: int
    metric: str
    lower_is_better: bool = False
    headline: tuple[str, ...] = ()


@dataclass(frozen=True)
class GameDefaults:
    # Episodes in an evaluation, each on its own fixed seed.
    eval_episodes: int
    # Steps after which an evaluation episode is cut short. A good Tetris
    # agent never tops out, so without a cap it would never finish.
    # Omatris's 2,500 pieces are 1,000 lines if every one is cleared: about
    # level 100, far enough for good agents to pull apart on score.
    eval_max_steps: int
    # Signals summed over an episode that `compare` shows beside the score.
    headline: tuple[str, ...]
    # Env settings every agent plays with, unless it or --env says otherwise.
    env: dict = field(default_factory=dict)
    # What the main test is called, and the tests every run also plays.
    main_test: str = "standard"
    tests: tuple[Test, ...] = ()

    def test(self, name: str | None) -> Test | None:
        """One of the other tests by name; None for the main test."""
        if not name or name == self.main_test:
            return None
        for test in self.tests:
            if test.name == name:
                return test
        known = ", ".join([self.main_test, *(t.name for t in self.tests)])
        raise ValueError(f"no test {name!r}; this game has: {known}")


_DEFAULTS = {
    # Ten key presses a second, a fast human: placing takes time, so gravity
    # pulls a piece while it moves and high levels are hard (the game's
    # README, "Placing at a human's speed"). Raw key presses keep their own
    # pace, frame_skip, and ignore it.
    "omatris": GameDefaults(
        eval_episodes=20, eval_max_steps=2500, headline=("lines",), env={"input_rate": 10}, main_test="marathon",
        tests=(
            # Getting out of trouble: each game opens on a dealt mess of holes
            # and overhangs, and is won the moment its last dealt row clears.
            # Ranked by pieces to win per point of the deal's Difficulty (the
            # number the app shows), so a hard deal is allowed more pieces and
            # "best game" means best played, not easiest dealt. A game lost (or
            # not won within the cap) counts as the whole cap. Learners never
            # train on it, so it also shows whether what they learned carries
            # over.
            Test("challenge", "clear a dealt mess in as few pieces as possible, for its difficulty",
                 {"mode": "challenge"}, episodes=20, max_steps=1000, metric="pieces_per_difficulty",
                 lower_is_better=True, headline=("steps_to_win", "difficulty", "won", "topped_out")),
        ),
    ),
    "omasnake": GameDefaults(eval_episodes=50, eval_max_steps=3000, headline=("ate",)),
}


def defaults(game: str) -> GameDefaults:
    return _DEFAULTS.get(game, GameDefaults(eval_episodes=20, eval_max_steps=1000, headline=()))
