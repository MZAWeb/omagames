import pytest


@pytest.fixture(autouse=True)
def experiments(tmp_path, monkeypatch):
    """Every test records into its own throwaway store, never omagym/experiments."""
    home = tmp_path / "experiments"
    monkeypatch.setenv("OMAGYM_HOME", str(home))
    return home
