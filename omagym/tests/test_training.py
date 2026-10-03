import json
from dataclasses import dataclass

import numpy as np

from omagym.agents.base import Agent
from omagym.cli import main
from omagym.env import Env
from omagym.store import Store
from omagym.training import Schedule, snapshot_files, train


class Counting(Agent):
    """A stand-in learner, so training is tested without PyTorch: it plays
    randomly and claims ten steps of progress per yield."""

    name = "counting"
    trainable = True

    @dataclass
    class Config:
        pass

    def __init__(self, config, spec, device="cpu"):
        super().__init__(config, spec, device)
        self.rng = np.random.default_rng(0)

    def act(self, obs, mask, explore=False):
        return int(self.rng.choice(np.flatnonzero(mask)))

    def train(self, ctx):
        steps = 0
        while True:
            steps += 10
            yield {"steps": steps}


def _training(store, snapshots, steps=100):
    with Env("omasnake") as probe:
        spec = probe.spec
    run = store.new_run("train", "omasnake", "counting", name="counting", agent_config={}, env_config={},
                        code="c", dirty=0, seed=0, device="cpu", eval_episodes=1, eval_max_steps=50)
    schedule = Schedule(steps=steps, eval_every=50, eval_episodes=1, final_episodes=1, max_steps=50,
                        snapshots=snapshots)
    train(store, run, Counting(Counting.Config(), spec), schedule, echo=lambda *_: None)
    return store.run(run["id"])


def test_snapshots_are_spread_evenly_from_the_start_to_the_end():
    assert Schedule(steps=90, eval_every=1, eval_episodes=1, final_episodes=1, max_steps=1,
                    snapshots=10).snapshot_steps() == list(range(0, 100, 10))
    assert Schedule(steps=90, eval_every=1, eval_episodes=1, final_episodes=1, max_steps=1,
                    snapshots=0).snapshot_steps() == []


def test_a_training_run_records_its_snapshots_in_order():
    store = Store()
    run = _training(store, snapshots=4, steps=90)
    files = snapshot_files(run)
    assert [f.name for f in files] == ["01.json", "02.json", "03.json", "04.json"]
    replays = [json.loads(f.read_text()) for f in files]
    assert [r["training_steps"] for r in replays] == [0, 30, 60, 90]
    assert replays[0]["agent"] == "counting after 0 steps"
    assert replays[-1]["agent"] == "counting after 90 steps"
    # The same game every time, so the snapshots show the same deal.
    assert len({r["seed"] for r in replays}) == 1
    assert [step for step, _ in store.series(run["id"], "snapshot/score")] == [0, 30, 60, 90]


def test_more_snapshots_than_progress_reports_are_not_duplicated():
    run = _training(Store(), snapshots=20, steps=50)
    steps = [json.loads(f.read_text())["training_steps"] for f in snapshot_files(run)]
    assert steps == sorted(set(steps)) and steps[0] == 0 and steps[-1] == 50


def test_watch_training_opens_every_snapshot_in_order(monkeypatch, capsys):
    store = Store()
    run = _training(store, snapshots=3, steps=60)
    launched = []
    monkeypatch.setattr("subprocess.call", lambda command: launched.append(command) or 0)
    assert main(["watch", run["id"], "--training"]) == 0
    command = launched[0]
    assert command[1] == "omasnake"
    assert command[2::2] == ["--replay"] * 3
    assert command[3::2] == [str(f) for f in snapshot_files(run)]
    assert "3 snapshots" in capsys.readouterr().out
    assert main(["show", run["id"]]) == 0
    assert "--training" in capsys.readouterr().out


def test_watch_training_needs_a_run_that_recorded_snapshots(capsys):
    assert main(["eval", "--game", "omasnake", "--agent", "random", "--episodes", "1", "--name", "e"]) == 0
    assert main(["watch", "e", "--training"]) == 2
    assert "recorded no snapshots" in capsys.readouterr().err
