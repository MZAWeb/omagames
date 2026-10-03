"""Every agent omagym knows, by name.

A name can be shared by agents for different games ("greedy" is one for
Tetris and one for Snake); resolve() picks the one for the game in hand.
"""

from __future__ import annotations

import importlib

from .base import Agent, make_config

_REGISTRY: dict[str, list[type[Agent]]] = {}
# Modules that could not be imported, and why: an agent needing PyTorch when
# it is not installed, say. Listed by `omagym agents` rather than crashing.
UNAVAILABLE: dict[str, str] = {}


def register(cls: type[Agent]) -> type[Agent]:
    for other in _REGISTRY.get(cls.name, []):
        if not cls.games or not other.games or set(cls.games) & set(other.games):
            raise ValueError(f"two agents called {cls.name!r} play the same game")
    _REGISTRY.setdefault(cls.name, []).append(cls)
    return cls


def resolve(name: str, game: str) -> type[Agent]:
    candidates = _REGISTRY.get(name, [])
    for cls in candidates:
        if not cls.games or game in cls.games:
            return cls
    if candidates:
        plays = sorted({g for cls in candidates for g in cls.games})
        raise ValueError(f"{name!r} plays {', '.join(plays)}, not {game}")
    hint = f" ({UNAVAILABLE[name]})" if name in UNAVAILABLE else ""
    raise ValueError(f"no agent called {name!r}{hint}; see `omagym agents`")


def all_agents() -> list[type[Agent]]:
    return [cls for name in sorted(_REGISTRY) for cls in _REGISTRY[name]]


# Importing an agent's module registers it. Add new agents' modules here.
for _module in ("random_agent", "greedy", "cem", "dqn", "lookahead", "mcts"):
    try:
        importlib.import_module(f"{__name__}.{_module}")
    except ImportError as error:
        UNAVAILABLE[_module] = f"needs {error.name}" if error.name else str(error)

__all__ = ["Agent", "UNAVAILABLE", "all_agents", "make_config", "register", "resolve"]
