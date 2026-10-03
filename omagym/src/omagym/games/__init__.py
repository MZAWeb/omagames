"""What omagym knows about each game beyond its env spec: how to evaluate it
fairly and which numbers to put beside the score when comparing runs."""

from __future__ import annotations

from dataclasses import dataclass


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


_DEFAULTS = {
    "omatris": GameDefaults(eval_episodes=20, eval_max_steps=2500, headline=("lines",)),
    "omasnake": GameDefaults(eval_episodes=50, eval_max_steps=3000, headline=("ate",)),
}


def defaults(game: str) -> GameDefaults:
    return _DEFAULTS.get(game, GameDefaults(eval_episodes=20, eval_max_steps=1000, headline=()))
