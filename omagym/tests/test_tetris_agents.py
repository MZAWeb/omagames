"""The Tetris agents beyond greedy: their features, and that each learns or
plans as it says. Kept small and quick; `omagym train` is where they get good."""

import numpy as np
import pytest

from omagym.agents import make_config, resolve
from omagym.env import Env
from omagym.evaluation import play
from omagym.games import omatris
from omagym.training import TrainContext


def _spec():
    with Env("omatris", actions="placement") as env:
        return env.spec


def _one_landing(board_rows: list[str], lines=0, spin=0):
    """An observation offering one landing, whose afterstate is drawn with
    '#' for filled cells, top row first, padded with empty rows above."""
    spec = _spec()
    rows, cols = spec.tensors["board"].shape
    drawn = np.array([[ch == "#" for ch in row] for row in board_rows], np.uint8)
    after = np.zeros((1, rows, cols), np.uint8)
    after[0, rows - len(board_rows):] = drawn
    candidates = np.zeros((1, len(spec.tensors["candidates"].labels)), np.int32)
    column = spec.tensors["candidates"].column
    candidates[0, column("valid")] = 1
    candidates[0, column("lines")] = lines
    candidates[0, column("spin")] = spin
    return {"afterstates": after, "candidates": candidates}, spec


def test_rich_features_measure_the_board_they_are_given():
    obs, spec = _one_landing([
        "##........",
        "#.#.......",
        "###.######",
    ], lines=4, spin=2)
    f = dict(zip(omatris.RICH, omatris.rich_features(obs, spec, 1)[0]))
    assert f["clear4"] == 1 and f["clear1"] == 0 and f["spin_full"] == 1
    assert f["holes"] == 1                       # column 1's middle cell, under its top
    assert f["max_height"] == 3
    assert f["height"] == 3 + 3 + 2 + 0 + 1 * 6  # columns 0 to 3, then six of height 1
    assert f["bumpiness"] == 0 + 1 + 2 + 1       # the steps between neighbouring columns
    assert f["wells"] == f["deepest_well"] == 1  # column 3, lower than both neighbours


def test_cem_improves_its_distribution_and_keeps_it(tmp_path):
    cls = resolve("cem", "omatris")
    config = make_config(cls, {"population": "6", "games": "1", "episode_steps": "20"})
    agent = cls(config, _spec())
    ctx = TrainContext("omatris", cls.env_config("omatris"), seed=0, device="cpu")
    generations = agent.train(ctx)
    first = next(generations)
    second = next(generations)
    assert first["generation"] == 1 and second["generation"] == 2
    assert second["steps"] > first["steps"] > 0
    assert np.any(agent.mean != 0)               # it moved away from knowing nothing

    agent.save(tmp_path / "best.pt")
    copy = cls(config, _spec())
    copy.load(tmp_path / "best.pt")
    assert copy.weights() == agent.weights()


def test_cem_weights_play_like_greedy_when_set_to_greedys():
    # A sanity check that the rich features are wired right: weights that
    # copy greedy's idea (clear lines, avoid holes and height) survive.
    cls = resolve("cem", "omatris")
    with Env("omatris", **cls.env_config("omatris"), max_steps=150) as env:
        agent = cls(cls.Config(), env.spec)
        weights = dict.fromkeys(omatris.RICH, 0.0)
        weights.update(clear1=1, clear2=2, clear3=3, clear4=4, holes=-4, height=-5, bumpiness=-2, topped_out=-100)
        agent.mean = np.array([weights[n] for n in omatris.RICH], np.float32)
        result = play(agent, env, seed=11)
    assert result["steps"] == 150 and result["sum_lines"] >= 50


def test_cem_refuses_an_objective_it_does_not_know():
    cls = resolve("cem", "omatris")
    with pytest.raises(ValueError, match="objective"):
        cls(make_config(cls, {"objective": "style"}), _spec())
