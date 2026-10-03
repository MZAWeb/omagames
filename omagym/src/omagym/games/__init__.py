"""What omagym knows about each game beyond its env spec: how to evaluate it
fairly and which numbers to put beside the score when comparing runs."""

from __future__ import annotations

from dataclasses import dataclass, field


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


_DEFAULTS = {
    # Ten key presses a second, a fast human: placing takes time, so gravity
    # pulls a piece while it moves and high levels are hard (the game's
    # README, "Placing at a human's speed"). Raw key presses keep their own
    # pace, frame_skip, and ignore it.
    "omatris": GameDefaults(eval_episodes=20, eval_max_steps=2500, headline=("lines",), env={"input_rate": 10}),
    "omasnake": GameDefaults(eval_episodes=50, eval_max_steps=3000, headline=("ate",)),
}


def defaults(game: str) -> GameDefaults:
    return _DEFAULTS.get(game, GameDefaults(eval_episodes=20, eval_max_steps=1000, headline=()))
