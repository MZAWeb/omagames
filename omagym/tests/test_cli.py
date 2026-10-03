import pytest

from omagym.cli import main


def test_eval_records_a_run_that_runs_show_and_compare_read(capsys):
    assert main(["eval", "--agent", "greedy", "--episodes", "2", "--max-steps", "40", "--name", "g"]) == 0
    assert main(["eval", "--agent", "random", "--episodes", "2", "--max-steps", "40", "--name", "r"]) == 0
    capsys.readouterr()
    assert main(["runs"]) == 0
    listing = capsys.readouterr().out
    assert "greedy" in listing and "random" in listing
    assert main(["compare", "g", "r"]) == 0
    assert "score" in capsys.readouterr().out
    assert main(["show", "last"]) == 0
    assert "evaluated on 2 fixed games" in capsys.readouterr().out


def test_the_same_agent_on_the_same_games_scores_the_same(capsys):
    for name in ("one", "two"):
        main(["eval", "--game", "omasnake", "--agent", "greedy", "--episodes", "3", "--name", name])
    capsys.readouterr()
    from omagym.store import Store

    store = Store()
    assert store.run("one")["summary"] == store.run("two")["summary"]


def test_mistakes_are_explained_not_raised(capsys):
    assert main(["eval", "--agent", "greedy", "--set", "hole=1"]) == 2
    assert "no setting 'hole'" in capsys.readouterr().err
    assert main(["eval", "--game", "omasnake", "--agent", "dqn"]) == 2
    assert main(["train", "--agent", "greedy", "--steps", "10"]) == 2
    assert "does not learn" in capsys.readouterr().err


def test_train_then_eval_the_checkpoint(capsys):
    pytest.importorskip("torch")
    settings = ["--set", "warmup=64", "--set", "batch=32", "--set", "episode_steps=40"]
    assert main(["train", "--agent", "dqn", "--steps", "300", "--eval-every", "150", "--eval-episodes", "1",
                 "--episodes", "2", "--max-steps", "30", "--name", "tiny", *settings]) == 0
    assert main(["eval", "--run", "tiny", "--episodes", "2", "--max-steps", "30", "--name", "tiny-eval"]) == 0
    from omagym.store import Store

    store = Store()
    trained, tested = store.run("tiny"), store.run("tiny-eval")
    assert tested["parent"] == trained["id"]
    assert tested["summary"]["score_mean"] == trained["summary"]["score_mean"]
    assert len(store.series(trained["id"], "eval/score_mean")) >= 2
