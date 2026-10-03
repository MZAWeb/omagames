"""A neural network that rates Tetris boards: the value of an afterstate.

An *afterstate* is the board a landing leaves behind, after its lines clear
and before the next piece arrives. Rating afterstates rather than actions is
the trick that makes value learning work for Tetris (README, Science 2.3): the
number of landings changes every piece, but every landing leaves a board,
and one network can rate any board. To choose, rate every landing's board
and take the best.

`dqn` learns such a network from its own play; `mcts` searches with one.
This module is what they share: how a board is turned into numbers for the
network (the *inputs*), and the network itself.
"""

from __future__ import annotations

import numpy as np
from torch import nn

from ...env import EnvSpec
from ...games import omatris

# The ways a board can be shown to the network, from most hand-made to least:
#
# - "features": the five numbers greedy uses (lines, holes, bumpiness,
#   height, topped out). Learns fast, but can only care about those five.
# - "rich": the fifteen numbers cem uses, which tell a Tetris from four
#   singles and see wells and transitions.
# - "board": every cell, 240 numbers, into a plain network. It has to find
#   its own features, which takes longer, and nothing caps what it can learn.
# - "rich_hand": "rich", plus what is in hand after the landing: the held
#   piece and the next one (omatris.hand_after()). Only this one lets the
#   network value keeping an I in the hold for a Tetris.
INPUTS = ("features", "rich", "board", "rich_hand")
_HAND = 2 * omatris.PIECE_KINDS

# Rough sizes of the five features on a busy board, so the network sees
# inputs around 0..1 whatever their units (networks learn badly when one
# input is in the hundreds and another a fraction).
_FEATURE_SCALE = np.array([4.0, 40.0, 60.0, 200.0, 1.0], np.float32)


def check(inputs: str) -> None:
    if inputs not in INPUTS:
        raise ValueError(f"inputs must be one of {', '.join(INPUTS)}, not {inputs!r}")


def _cells(spec: EnvSpec) -> int:
    return int(np.prod(spec.tensors["afterstates"].shape[1:]))


def input_size(inputs: str, spec: EnvSpec) -> int:
    """How many numbers describe one board (and, with _hand, what's in hand)."""
    base = {"features": len(omatris.FEATURES), "rich": len(omatris.RICH)}.get(inputs.removesuffix("_hand"))
    size = base if base is not None else _cells(spec)
    return size + (_HAND if inputs.endswith("_hand") else 0)


def inputs_for(inputs: str, obs: dict[str, np.ndarray], spec: EnvSpec, count: int) -> np.ndarray:
    """[count, input_size] floats: every landing on offer, as the network sees it."""
    if inputs == "features":
        return omatris.afterstate_features(obs, spec, count) / _FEATURE_SCALE
    if inputs.endswith("_hand"):
        board = inputs_for(inputs.removesuffix("_hand"), obs, spec, count)
        return np.concatenate([board, omatris.hand_after(obs, spec, count)], axis=1)
    if inputs == "rich":
        return omatris.rich_features(obs, spec, count) / omatris.RICH_SCALE
    # The cells themselves, 0 or 1, flattened.
    return obs["afterstates"][:count].reshape(count, -1).astype(np.float32)


def network(inputs: str, spec: EnvSpec, hidden: int, layers: int) -> nn.Module:
    """A network from one board's numbers to one number: how good the board is.

    `layers` fully connected layers of `hidden` units, each followed by a
    ReLU (which lets the network bend: without it, any stack of layers is
    just one weighted sum), then one output.
    """
    stack: list[nn.Module] = []
    width = input_size(inputs, spec)
    for _ in range(layers):
        stack += [nn.Linear(width, hidden), nn.ReLU()]
        width = hidden
    stack.append(nn.Linear(width, 1))
    return nn.Sequential(*stack)
