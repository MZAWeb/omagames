"""Monte Carlo tree search with a learned value, in the spirit of AlphaZero.

**The idea.** `lookahead` searches a fixed shape: the best 8 lines, one piece
deeper each time. MCTS lets the search decide where to look. It grows a tree
of possible futures one *simulation* at a time, and each simulation goes
deeper down the lines that look best so far, while still now and then
trying the ones it knows least about. Spend more simulations and the move
gets better; that is its whole appeal.

**One simulation** has three parts:

1. **Select.** From the root (the real game now), step down the tree,
   choosing at each node the landing with the best mix of "looks good" and
   "barely tried" (the PUCT rule in _select()).
2. **Evaluate.** On reaching a landing never tried before, ask the network
   what it is worth: its rating of the board that landing leaves. (A
   landing tried once already is *expanded*: played out on a copy of the
   game, so the next simulation through it can look at the piece after.)
3. **Back up.** Carry that value back up the path, so every landing on the
   way averages in what was found below it.

After the simulations, play the landing visited most: the search kept
coming back to it, which is a steadier signal than any single value.

**Learning.** The network starts knowing nothing, and so does the search.
But a search with N simulations knows more than the network alone, so the
network is trained to predict what the search found: the searched value of
the move it played. Better network, better search; better search, better
targets. That loop is AlphaZero's (Silver et al., 2018), and the reason
search and learning together beat either alone.

**Fresh tree every move.** AlphaZero keeps the subtree under the move it
played. Here the copies in the tree dealt their own pieces beyond the
preview (to stay honest, see base.decide()), so once the real game deals
its next piece, the tree's guesses below that point are wrong. The search
starts again from the real game.

**It is slow.** Every simulation that expands copies the game, plays a
move and rates the new landings, about a millisecond. Evaluate it with
fewer games (`--episodes 5`), and start it from a trained dqn network
(`--set model=<dqn run>`) rather than from nothing.
"""

from __future__ import annotations

import copy
from dataclasses import dataclass, field
from pathlib import Path

import numpy as np
import torch
from torch import nn

from ...env import Env
from ...games import omatris
from .. import register
from . import afterstate_value
from ..base import Agent


@register
class MonteCarloTreeSearch(Agent):
    name = "mcts"
    games = ("omatris",)
    description = "Monte Carlo tree search guided by a value network that learns from the search (AlphaZero-style)."
    trainable = True
    default_steps = 20_000

    @dataclass
    class Config:
        # Simulations per move: the search's budget. More is stronger and slower.
        simulations: int = 32
        # How each simulation picks the landing to try, at every node:
        # "puct" (AlphaZero's rule, below) or "thompson" (Thompson sampling,
        # see _thompson()).
        selection: str = "puct"
        # How much "barely tried" counts against "looks good" in PUCT.
        c_puct: float = 1.5
        # Thompson sampling's uncertainty about a landing never tried, on the
        # 0..1 value scale; it shrinks with every visit.
        ts_spread: float = 0.5
        # How sharply the network's ratings turn into the priors that tell
        # the search where to look first: smaller means more single-minded.
        prior_temperature: float = 0.5
        # A trained dqn run to take the network from (its inputs and size
        # replace the three below). Empty starts from an untrained network.
        model: str = ""
        inputs: str = "features"
        hidden: int = 64
        layers: int = 2
        # Training: as in dqn.
        lr: float = 1e-3
        gamma: float = 0.95
        buffer: int = 20_000
        batch: int = 256
        warmup: int = 500
        episode_steps: int = 300
        # The rewards the search adds up, as in dqn (whose settings these mirror,
        # so a network from a dqn run keeps meaning the same thing).
        reward_piece: float = 1.0
        reward_line: float = 10.0
        reward_top_out: float = -20.0
        reward_win: float = 0.0         # clearing a Challenge (try 100)
        reward_dealt_row: float = 0.0   # per dealt row cleared (try 10)
        reward_tspin: float = 0.0
        seed: int = 0

    @classmethod
    def env_config(cls, game: str) -> dict:
        return {"actions": "placement"}

    def __init__(self, config: Config, spec, device: str = "cpu"):
        super().__init__(config, spec, device)
        torch.manual_seed(config.seed)
        self.rng = np.random.default_rng(config.seed)
        self.inputs, hidden, layers = config.inputs, config.hidden, config.layers
        weights = None
        if config.model:
            self.inputs, hidden, layers, weights = _dqn_network(config.model)
        afterstate_value.check(self.inputs)
        if config.selection not in ("puct", "thompson"):
            raise ValueError(f"selection must be puct or thompson, not {config.selection!r}")
        self.net = afterstate_value.network(self.inputs, spec, hidden, layers).to(device)
        if weights is not None:
            self.net.load_state_dict(weights)
        self.optimizer = torch.optim.Adam(self.net.parameters(), lr=config.lr)
        self.steps = 0

    def act(self, obs, mask, explore=False) -> int:
        raise TypeError("mcts searches with the game itself; the framework calls decide(env, ...)")

    # -- one search ------------------------------------------------------------

    def decide(self, env: Env, obs, mask, explore=False) -> int:
        return self._search(env, obs, mask, explore)[1]

    def _search(self, env: Env, obs, mask, explore: bool) -> tuple[_Node, int]:
        """Runs the simulations from the real game; the root, and the move to play."""
        self._env, self._copies, self._bounds = env, [], _Bounds()
        root = self._node(None, obs, mask, ended=False)
        if explore:
            # While training, a little random noise on the root's priors makes
            # the search try moves the network would never suggest, which is
            # how it finds out the network was wrong (AlphaZero does the same).
            noise = self.rng.dirichlet([0.3] * len(root.priors))
            root.priors = 0.75 * root.priors + 0.25 * noise
        for _ in range(self.config.simulations):
            self._simulate(root)
        for env_copy in self._copies:
            env_copy.close()

        if explore:
            # Training: sample a move in proportion to its visits, so the agent
            # sees, and learns about, more than one way to play.
            move = int(self.rng.choice(len(root.visits), p=root.visits / root.visits.sum()))
        else:
            # The most visited, ties going to the better value.
            move = int(np.lexsort((root.q(), root.visits))[-1])
        return root, move

    def _simulate(self, root: _Node) -> None:
        # 1. Select: walk down until a landing never tried, or a game over.
        path: list[tuple[_Node, int]] = []
        node = root
        while True:
            landing = self._select(node)
            path.append((node, landing))
            if node.visits[landing] == 0:
                # 2. Evaluate: the network's rating of that landing's board.
                value = float(node.ratings[landing])
                break
            if landing not in node.children:
                # Tried once already: play it out on a copy, so the search
                # can now look at what comes after it.
                node.children[landing] = self._expand(node, landing)
            child = node.children[landing]
            if child.ended:
                value = node.rewards[landing]   # nothing comes after a game over
                break
            node = child

        # 3. Back up. A landing is worth its own reward plus the discounted
        # value of what came after it. Walking up from the leaf, each landing
        # gets the value found below it, then passes its own reward plus that,
        # discounted, to the landing above.
        for depth in range(len(path) - 1, -1, -1):
            node, landing = path[depth]
            node.visits[landing] += 1
            node.totals[landing] += value
            self._bounds.update(node.totals[landing] / node.visits[landing])
            if depth > 0:
                parent, parent_landing = path[depth - 1]
                value = parent.rewards[parent_landing] + self.config.gamma * value

    def _select(self, node: _Node) -> int:
        """PUCT: the landing with the most value plus exploration bonus.

        value: how good the landing looks so far (scaled to 0..1 by the
            best and worst seen in this search, since rewards have no fixed
            range).
        bonus: c_puct * prior * sqrt(visits to this node) / (1 + visits to
            the landing). Large for a landing the network likes and the
            search has hardly tried; shrinks every time it is tried. So
            early on the priors steer, and later the values found do.
        """
        if self.config.selection == "thompson":
            return self._thompson(node)
        value = self._bounds.scale(node.q())
        bonus = self.config.c_puct * node.priors * np.sqrt(node.visits.sum() + 1) / (1 + node.visits)
        return int(np.argmax(value + bonus))

    def _thompson(self, node: _Node) -> int:
        """Thompson sampling: draw a plausible value for every landing, try the best draw.

        The search treats each landing as a slot machine whose payout it is
        unsure of: a Normal around the value found so far (or the network's
        guess, if never tried), narrower the more it has been tried. Drawing
        once from each and taking the highest draw tries a landing exactly as
        often as it might be the best one. A well-tried good landing is
        usually picked; a barely tried one with a fair guess gets a chance
        now and then; a well-tried bad one, almost never. It is the bandit
        rule PUCT's bonus is an alternative to (README, To do 1), with no
        c_puct to tune, though the network's priors play no part in it.
        """
        value = self._bounds.scale(node.q())
        spread = self.config.ts_spread / np.sqrt(1 + node.visits)
        return int(np.argmax(self.rng.normal(value, spread)))

    def _expand(self, node: _Node, landing: int) -> _Node:
        source = node.env or self._env
        env_copy = source.clone(reseed_hidden=True, seed=int(self.rng.integers(2**31)))
        self._copies.append(env_copy)
        step = env_copy.step(landing)
        node.rewards[landing] = self._reward(step, node.dealt_left)
        return self._node(env_copy, step.obs, env_copy.mask(), ended=step.done)

    def _node(self, env: Env | None, obs, mask, ended: bool) -> _Node:
        """A decision point: the landings on offer, rated by the network."""
        dealt_left = int(obs["stats"][self.spec.tensors["stats"].column("dealt_rows_left")])
        if ended:
            return _Node(env, np.zeros((0, 1), np.float32), np.zeros(0, np.float32), np.zeros(0), ended=True,
                         dealt_left=dealt_left)
        inputs = afterstate_value.inputs_for(self.inputs, obs, self.spec, omatris.landing_count(mask))
        ratings = self._values(inputs)
        # Priors: the ratings, standardised and softened into probabilities
        # (a softmax). The landings the network rates best get looked at first.
        spread = ratings.std() + 1e-6
        logits = (ratings - ratings.max()) / spread / self.config.prior_temperature
        priors = np.exp(logits) / np.exp(logits).sum()
        return _Node(env, inputs, ratings, priors, dealt_left=dealt_left)

    def _values(self, inputs: np.ndarray) -> np.ndarray:
        with torch.no_grad():
            return self.net(torch.as_tensor(inputs, device=self.device)).squeeze(1).cpu().numpy()

    def _reward(self, step, dealt_before: int) -> float:
        c, s = self.config, step.signals
        reward = c.reward_piece + c.reward_line * s["lines"] ** 2 + c.reward_tspin * (s["tspin"] > 0)
        if s["topped_out"]:
            reward += c.reward_top_out
        elif step.terminated:
            reward += c.reward_win   # ended without topping out: the goal was reached
        dealt_after = int(step.obs["stats"][self.spec.tensors["stats"].column("dealt_rows_left")])
        return reward + c.reward_dealt_row * (dealt_before - dealt_after)

    # -- learning from the search -----------------------------------------------

    def train(self, ctx):
        c = self.config
        env = ctx.make_env(max_steps=c.episode_steps)
        width = afterstate_value.input_size(self.inputs, self.spec)
        boards, targets = np.zeros((c.buffer, width), np.float32), np.zeros(c.buffer, np.float32)
        stored, losses = 0, []
        while True:
            obs, moves = env.reset(ctx.next_seed()), 0
            while True:
                root, move = self._search(env, obs, env.mask(), explore=True)
                # The lesson: this board is worth what the search found it worth.
                slot = stored % c.buffer
                boards[slot], targets[slot] = root.inputs[move], root.q()[move]
                stored += 1
                step = env.step(move)
                self.steps += 1
                moves += 1
                if stored >= c.warmup:
                    losses.append(self._learn(boards, targets, min(stored, c.buffer)))
                if step.done:
                    break
                obs = step.obs
            info = env.info()
            yield {"steps": self.steps, "episode_score": info["score"], "episode_lines": info["lines"],
                   "moves": moves, "loss": float(np.mean(losses)) if losses else float("nan")}
            losses.clear()

    def _learn(self, boards: np.ndarray, targets: np.ndarray, size: int) -> float:
        """One step of plain supervised learning: predict the searched values."""
        rows = self.rng.integers(size, size=min(self.config.batch, size))
        x = torch.as_tensor(boards[rows], device=self.device)
        y = torch.as_tensor(targets[rows], device=self.device)
        loss = nn.functional.mse_loss(self.net(x).squeeze(1), y)
        self.optimizer.zero_grad()
        loss.backward()
        self.optimizer.step()
        return loss.item()

    def save(self, path: Path) -> None:
        torch.save({"net": self.net.state_dict(), "steps": self.steps}, path)

    def load(self, path: Path) -> None:
        state = torch.load(path, map_location=self.device)
        self.net.load_state_dict(state["net"])
        self.steps = state["steps"]


@dataclass(eq=False)
class _Node:
    """A decision point in the tree: a game with a piece to place.

    Per landing on offer it keeps the network's first guess (`ratings`), how
    often the search tried it (`visits`) and the values it found adding up
    (`totals`), and once expanded, the reward it earned and the node after it.
    """

    env: Env | None                 # a copy of the game here; None at the root (the real game)
    inputs: np.ndarray              # each landing's board, as the network sees it
    ratings: np.ndarray             # the network's value of each landing
    priors: np.ndarray              # where to look first, from the ratings
    ended: bool = False
    dealt_left: int = 0             # a Challenge's dealt rows still to clear here
    visits: np.ndarray = field(init=False)
    totals: np.ndarray = field(init=False)
    rewards: dict[int, float] = field(default_factory=dict)
    children: dict[int, _Node] = field(default_factory=dict)

    def __post_init__(self):
        self.visits = np.zeros(len(self.ratings), np.int64)
        self.totals = np.zeros(len(self.ratings), np.float64)

    def q(self) -> np.ndarray:
        """Each landing's value: the average found, or the network's guess if never tried."""
        return np.where(self.visits > 0, self.totals / np.maximum(self.visits, 1), self.ratings)


class _Bounds:
    """The lowest and highest value seen in a search, to put values on a 0..1
    scale the exploration bonus can be weighed against (as MuZero does)."""

    def __init__(self):
        self.low, self.high = np.inf, -np.inf

    def update(self, value: float) -> None:
        self.low, self.high = min(self.low, value), max(self.high, value)

    def scale(self, values: np.ndarray) -> np.ndarray:
        if self.high <= self.low:
            return np.zeros_like(values)
        return (values - self.low) / (self.high - self.low)


def _dqn_network(model: str):
    """A trained dqn's inputs, size and weights."""
    from ...store import Store

    run = Store().run(model)
    if run["kind"] != "train" or run["agent"] != "dqn":
        raise ValueError(f"model must be a dqn training run; {run['id']} is a {run['kind']} of {run['agent']}")
    settings = run["agent_config"]
    state = torch.load(Path(run["dir"]) / "best.pt", map_location="cpu")
    return settings["inputs"], settings["hidden"], settings["layers"], copy.deepcopy(state["net"])
