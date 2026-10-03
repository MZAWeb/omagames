import pytest

from omagym import report
from omagym.comparison import rank


def _run(run_id, name=None, cap=100, game="omatris", env=None):
    return {"id": run_id, "name": name, "game": game, "agent": run_id, "kind": "eval", "status": "done",
            "eval_max_steps": cap, "env_config": env or {"actions": "placement"}}


def _games(scores, lines=None, first_seed=0):
    lines = lines or [0] * len(scores)
    return [{"episode": i, "seed": first_seed + i, "score": s, "steps": 10, "sum_lines": n}
            for i, (s, n) in enumerate(zip(scores, lines))]


def test_the_best_leads_and_a_clear_loser_is_called_worse():
    runs = [_run("weak"), _run("strong")]
    ranking = rank(runs, {"weak": _games([10, 12, 11, 9]), "strong": _games([20, 21, 19, 22])})
    best, other = ranking.standings
    assert best.run["id"] == "strong" and best.verdict == "best"
    assert other.verdict == "worse" and (other.wins, other.ties, other.losses) == (0, 0, 4)
    assert other.difference == pytest.approx(-10.0)
    assert other.low <= other.difference <= other.high < 0


def test_a_difference_within_the_noise_is_not_called():
    runs = [_run("a"), _run("b")]
    ranking = rank(runs, {"a": _games([10, 30, 10, 30]), "b": _games([30, 10, 30, 11])})
    assert ranking.standings[1].verdict == "can't tell"
    assert ranking.standings[1].low < 0 < ranking.standings[1].high


def test_identical_play_is_the_same_not_worse():
    runs = [_run("a"), _run("b")]
    ranking = rank(runs, {"a": _games([5, 6]), "b": _games([5, 6])})
    assert ranking.standings[1].verdict == "same"


def test_the_metric_decides_the_order():
    runs = [_run("scorer"), _run("clearer")]
    episodes = {"scorer": _games([90, 90], lines=[1, 1]), "clearer": _games([10, 10], lines=[4, 4])}
    assert rank(runs, episodes).standings[0].run["id"] == "scorer"
    assert rank(runs, episodes, "lines").standings[0].run["id"] == "clearer"
    with pytest.raises(ValueError, match="no metric 'holes'.*lines"):
        rank(runs, episodes, "holes")


def test_when_lower_is_better_the_lowest_leads():
    runs = [_run("tall"), _run("flat")]
    episodes = {"tall": _games([9, 8, 9, 9]), "flat": _games([2, 3, 2, 2])}
    best, other = rank(runs, episodes, lower_is_better=True).standings
    assert best.run["id"] == "flat"
    assert other.verdict == "worse" and other.losses == 4 and other.difference > 0
    assert "lower is better" in report.ranking(rank(runs, episodes, lower_is_better=True))


def test_only_games_every_run_played_count():
    runs = [_run("a"), _run("b")]
    ranking = rank(runs, {"a": _games([1, 2, 3]), "b": _games([5, 5], first_seed=1)})
    assert ranking.seeds == [1, 2]
    assert ranking.standings[0].run["id"] == "b"


def test_runs_that_did_not_play_the_same_test_are_refused():
    with pytest.raises(ValueError, match="different games"):
        rank([_run("a"), _run("b", game="omasnake")], {"a": _games([1]), "b": _games([1])})
    with pytest.raises(ValueError, match="cut at different lengths"):
        rank([_run("a"), _run("b", cap=200)], {"a": _games([1]), "b": _games([1])})
    with pytest.raises(ValueError, match="no game in common"):
        rank([_run("a"), _run("b")], {"a": _games([1]), "b": _games([1], first_seed=5)})
    with pytest.raises(ValueError, match="no evaluation results"):
        rank([_run("a"), _run("b")], {"a": _games([1])})


def test_the_report_names_the_winner_and_flags_different_env_settings():
    runs = [_run("a", name="greedy-x"), _run("b", name="dqn-x", env={"actions": "drop"})]
    text = report.ranking(rank(runs, {"a": _games([1, 2, 1, 2]), "b": _games([9, 9, 8, 9])}))
    assert "dqn-x played best." in text
    assert "beyond doubt on these games: greedy-x" in text
    assert "env settings differ (actions)" in text


def test_runs_sharing_a_name_are_shown_by_id():
    runs = [_run("first", name="baseline"), _run("second", name="baseline")]
    text = report.ranking(rank(runs, {"first": _games([1, 2]), "second": _games([1, 2])}))
    assert "first played best." in text and "on every game: second." in text
