import subprocess

import pytest

from omagym import provenance, report
from omagym.native import REPO_ROOT
from omagym.store import Store


def _run(store, **fields):
    base = dict(agent_config={"x": 1}, env_config={"mode": "zen"}, code="abc", dirty=0,
                eval_episodes=5, eval_max_steps=100)
    return store.new_run("eval", "omatris", "greedy", **{**base, **fields})


def test_a_run_keeps_everything_recorded_about_it(experiments):
    store = Store()
    run = _run(store, name="baseline")
    store.log(run["id"], 10, {"loss": 0.5, "steps": 10})
    store.add_episodes(run["id"], [{"episode": 0, "seed": 1, "score": 7}])
    store.set_summary(run["id"], {"score_mean": 7.0, "score_std": 0.0, "episodes": 1})
    store.finish(run["id"], "done")

    again = Store().run("baseline")
    assert again["id"] == run["id"] and again["status"] == "done"
    assert again["agent_config"] == {"x": 1} and again["summary"]["score_mean"] == 7.0
    assert store.series(run["id"], "loss") == [(10, 0.5)]
    assert store.episodes(run["id"])[0]["score"] == 7
    assert (experiments / "experiments.sqlite").exists()


def test_runs_are_found_by_prefix_name_or_last():
    store = Store()
    first, second = _run(store), _run(store, name="second")
    assert store.run("last")["id"] == second["id"]
    assert store.run(first["id"])["id"] == first["id"]
    assert second["id"] != first["id"]
    with pytest.raises(KeyError, match="could be any of"):
        store.run(first["id"][:8])
    with pytest.raises(KeyError, match="no run"):
        store.run("nothing-like-it")


def test_diff_shows_what_differs_and_warns_on_different_tests():
    store = Store()
    a = _run(store, agent_config={"holes": -0.3})
    b = _run(store, agent_config={"holes": -1.0}, eval_max_steps=200)
    for run in (a, b):
        store.set_summary(run["id"], {"score_mean": 1.0, "score_std": 0.0, "episodes": 5})
    text = report.diff([store.run(a["id"]), store.run(b["id"])])
    assert "holes" in text and "-1.0" in text
    assert "not tested on the same games" in text


def test_the_code_a_run_ran_is_recorded(tmp_path):
    code = provenance.snapshot(tmp_path)
    head = subprocess.run(["git", "-C", str(REPO_ROOT), "rev-parse", "HEAD"], capture_output=True, text=True)
    assert code["commit_sha"] == head.stdout.strip()
    assert code["code"].startswith(head.stdout.strip()[:10])
    assert bool(code["dirty"]) == (tmp_path / "code.patch").exists()
