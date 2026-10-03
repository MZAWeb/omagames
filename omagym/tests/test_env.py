import numpy as np
import pytest

from omagym import native
from omagym.env import Env


@pytest.mark.parametrize("game", ["omatris", "omasnake"])
def test_both_games_have_an_env(game):
    assert game in native.games()
    with Env(game) as env:
        assert env.spec.game == game
        assert env.spec.rules_version >= 1
        obs = env.reset(1)
        assert set(obs) == set(env.spec.tensors)
        mask = env.mask()
        assert mask.dtype == bool and mask.any()
        step = env.step(int(np.flatnonzero(mask)[0]))
        assert set(step.signals) == set(env.spec.signals)


def test_tensors_have_their_spec_shapes_and_labels():
    with Env("omatris", actions="placement") as env:
        obs = env.reset(3)
        for name, tensor in env.spec.tensors.items():
            assert obs[name].shape == tensor.shape
            assert obs[name].dtype == tensor.dtype
        assert env.spec.tensors["candidates"].column("lines") == 6


def test_observations_are_the_agents_to_keep():
    with Env("omatris") as env:
        first = env.reset(3)
        board = first["board"].copy()
        env.step(int(np.flatnonzero(env.mask())[0]))
        assert (first["board"] == board).all()


def test_a_bad_config_or_action_says_why():
    with pytest.raises(native.EnvError, match="unknown config key"):
        Env("omatris", acitons="raw")
    with Env("omasnake", actions="absolute") as env:
        env.reset(1)
        with pytest.raises(native.EnvError, match="masked"):
            env.step(2)  # left, back onto the neck


def test_max_steps_cuts_an_episode_short():
    with Env("omasnake", max_steps=3) as env:
        env.reset(1)
        steps = [env.step(0) for _ in range(3)]
        assert steps[-1].truncated and not steps[-1].terminated


def test_a_clone_plays_on_identically_and_the_replay_is_the_game():
    # Straight on in Wrap never dies, so both copies play the whole way.
    with Env("omasnake", mode="wrap") as env:
        env.reset(5)
        for _ in range(10):
            env.step(0)
        twin = env.clone()
        for _ in range(40):
            a, b = env.step(0), twin.step(0)
            assert (a.obs["grid"] == b.obs["grid"]).all()
        replay = env.replay()
        assert replay["format"] == "replay/v1" and replay["game"] == "omasnake" and replay["seed"] == 5
        twin.close()
