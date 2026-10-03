import numpy as np
import pytest

from omagym.agents import all_agents, make_config, resolve
from omagym.env import Env
from omagym.evaluation import play


def _games(cls):
    return cls.games or ("omatris", "omasnake")


@pytest.mark.parametrize("cls", all_agents(), ids=lambda c: f"{c.name}-{'-'.join(c.games) or 'any'}")
def test_every_agent_only_ever_picks_legal_actions(cls):
    for game in _games(cls):
        with Env(game, **cls.env_config(game)) as env:
            agent = cls(cls.Config(), env.spec)
            obs = env.reset(7)
            for _ in range(60):
                mask = env.mask()
                action = agent.act(obs, mask)
                assert mask[action]
                step = env.step(action)
                if step.done:
                    obs = env.reset(8)
                else:
                    obs = step.obs


def test_names_resolve_per_game():
    assert resolve("greedy", "omatris").games == ("omatris",)
    assert resolve("greedy", "omasnake").games == ("omasnake",)
    assert resolve("random", "omasnake").name == "random"
    with pytest.raises(ValueError, match="no agent"):
        resolve("genius", "omatris")


def test_settings_are_typed_and_checked():
    cls = resolve("greedy", "omatris")
    assert make_config(cls, {"holes": "-1.5"}).holes == -1.5
    with pytest.raises(ValueError, match="no setting 'hole'"):
        make_config(cls, {"hole": "1"})


def test_greedy_tetris_clears_lines_and_random_does_not():
    results = {}
    for name in ("greedy", "random"):
        cls = resolve(name, "omatris")
        with Env("omatris", **cls.env_config("omatris"), max_steps=150) as env:
            results[name] = play(cls(cls.Config(), env.spec), env, seed=11)
    assert results["greedy"]["sum_lines"] >= 50
    assert results["greedy"]["steps"] == 150  # still going when cut
    assert results["random"]["sum_lines"] <= 2


def test_greedy_snake_eats():
    cls = resolve("greedy", "omasnake")
    with Env("omasnake", **cls.env_config("omasnake")) as env:
        result = play(cls(cls.Config(), env.spec), env, seed=3)
    assert result["sum_ate"] >= 10


def test_dqn_learns_a_little_and_keeps_what_it_learned(tmp_path):
    pytest.importorskip("torch")
    from omagym.training import TrainContext

    cls = resolve("dqn", "omatris")
    config = make_config(cls, {"warmup": "64", "batch": "32", "episode_steps": "40", "target_sync": "50"})
    with Env("omatris", **cls.env_config("omatris")) as env:
        agent = cls(config, env.spec)
        ctx = TrainContext("omatris", cls.env_config("omatris"), seed=0, device="cpu")
        progress = []
        for update in agent.train(ctx):
            progress.append(update)
            if update["steps"] >= 200:
                break
        assert progress[-1]["steps"] >= 200 and np.isfinite(progress[-1]["loss"])

        agent.save(tmp_path / "agent.pt")
        copy = cls(config, env.spec)
        copy.load(tmp_path / "agent.pt")
        obs = env.reset(9)
        assert copy.act(obs, env.mask()) == agent.act(obs, env.mask())
