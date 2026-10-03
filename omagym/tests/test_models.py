from pathlib import Path

from omagym.cli import main
from omagym.store import Store


def _trained(store, name, steps_taken, budget=100, checkpoint=True):
    run = store.new_run("train", "omatris", "dqn", name=name, agent_config={}, env_config={}, code="c",
                        dirty=0, train_steps=budget, eval_episodes=2, eval_max_steps=10)
    for step in range(10, steps_taken + 1, 10):
        store.log(run["id"], step, {"train/loss": 1.0})
    store.set_summary(run["id"], {"score_mean": 5.0, "score_std": 1.0, "episodes": 2})
    store.finish(run["id"], "done" if steps_taken == budget else "interrupted")
    if checkpoint:
        (Path(run["dir"]) / "best.pt").write_bytes(b"weights")
    return run


def _test_of(store, model, name):
    run = store.new_run("eval", "omatris", "dqn", name=name, parent=model["id"], agent_config={},
                        env_config={}, code="c", dirty=0, eval_episodes=2, eval_max_steps=10)
    store.set_summary(run["id"], {"score_mean": 6.0, "score_std": 0.5, "episodes": 2})
    return run


def test_models_lists_training_runs_with_a_checkpoint(capsys):
    store = Store()
    _trained(store, "finished", 100)
    _trained(store, "stopped-early", 40)
    _trained(store, "no-weights", 100, checkpoint=False)
    store.new_run("eval", "omatris", "greedy", name="not-a-model", agent_config={}, env_config={}, code="c", dirty=0)
    assert main(["models"]) == 0
    text = capsys.readouterr().out
    assert "finished" in text and "stopped-early" in text
    assert "no-weights" not in text and "not-a-model" not in text
    assert "100 steps" in text and "40 steps" in text  # what was trained, not what was asked for


def test_no_models_yet_says_how_to_get_one(capsys):
    assert main(["models"]) == 0
    assert "no trained models yet" in capsys.readouterr().out


def test_show_of_a_model_lists_its_checkpoint_and_its_tests(capsys):
    store = Store()
    model = _trained(store, "m", 40)
    _test_of(store, model, "m-on-100-games")
    assert main(["show", "m"]) == 0
    text = capsys.readouterr().out
    assert "trained   40 steps of 100 asked for" in text
    assert "best.pt" in text
    assert "tests of its checkpoint" in text and "m-on-100-games" in text


def test_a_model_is_deleted_with_its_tests_only_when_asked(capsys):
    store = Store()
    model = _trained(store, "m", 100)
    _test_of(store, model, "t")
    assert main(["delete", "m", "--yes"]) == 2
    assert "--with-tests" in capsys.readouterr().err
    assert main(["delete", "m", "--yes", "--with-tests"]) == 0
    assert "deleted 2 run(s)" in capsys.readouterr().out
    assert Store().runs() == []
