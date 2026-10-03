"""The cross-entropy method (CEM): evolving the weights of a board evaluation.

**The idea.** Like `greedy`, this agent rates every landing on offer with a
weighted sum of features of the board it would leave, and takes the best.
Unlike greedy, nobody picks the weights: CEM searches for them.

It keeps a *distribution* over weight vectors, a Gaussian with a mean and a
spread (standard deviation) per weight, and improves it one *generation* at
a time:

1. **Sample** a population of weight vectors from the Gaussian.
2. **Score** each by letting it play a few games.
3. **Select** the elite, the best fifth or so.
4. **Refit** the Gaussian to the elite: its new mean is their average, its new
   spread is how much they still disagree.

Over generations the mean drifts toward weights that play well, and the
spread shrinks as the elite agree. Nothing here knows Tetris or computes a
gradient: CEM only ever sees "these numbers scored that much". That is
black-box optimisation (SCIENCE.md, section 2.2).

**The one trick that matters: noise.** Left alone, the spread collapses
within a few generations, long before the mean is any good, and the search
stops exploring. Szita and Lőrincz (2006) fixed that by adding a little
extra spread every generation, decreasing over time. With it, CEM clears
hundreds of thousands of lines of classic Tetris with features like these;
without it, a few hundred.

**Why it suits Tetris.** A good Tetris evaluation is a handful of numbers,
and CEM is very good at finding a handful of numbers. It also optimises the
game's real objective directly (`objective = "score"`): no reward to design,
no discounting, no value estimates to get wrong.

**Read next:** `lookahead.py`, which searches with weights found here.
"""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path

import numpy as np

from ..games import omatris
from . import register
from .base import Agent


@register
class CrossEntropy(Agent):
    name = "cem"
    games = ("omatris",)
    description = "Evolves the weights of 15 board features with the noisy cross-entropy method. No PyTorch."
    trainable = True
    default_steps = 500_000

    @dataclass
    class Config:
        # Weight vectors tried each generation. More is a better estimate of
        # where the good weights are, and a slower generation.
        population: int = 50
        # The fraction of the population kept as the elite.
        elite: float = 0.2
        # The starting spread of every weight. Features are scaled (see
        # omatris.RICH_SCALE), so 1 means "anything from strongly against to
        # strongly for this feature".
        init_std: float = 1.0
        # The extra spread added each generation, fading to nothing over
        # `noise_generations`. This is what keeps the search from stalling.
        noise: float = 0.3
        noise_generations: int = 40
        # Games each candidate plays per generation, and where each is cut.
        # Every candidate plays the *same* games (see train()).
        games: int = 2
        episode_steps: int = 300
        # What "playing well" means: the game's own "score", or "lines"...
        objective: str = "score"
        # ...plus bonuses, in the objective's units. A Challenge ends when it
        # is won, so on score alone winning *costs* the points it would have
        # gone on to make: with Challenge games in the training mix, give a
        # win and the digging something (try 50000 and 2000 with score).
        fitness_win: float = 0.0
        fitness_dealt_row: float = 0.0  # per dealt row cleared
        seed: int = 0

    @classmethod
    def env_config(cls, game: str) -> dict:
        # One action per landing, the setting every feature-based agent uses.
        return {"actions": "placement"}

    def __init__(self, config: Config, spec, device: str = "cpu"):
        super().__init__(config, spec, device)
        if config.objective not in ("score", "lines"):
            raise ValueError(f"objective must be score or lines, not {config.objective!r}")
        self.rng = np.random.default_rng(config.seed)
        # The distribution being improved. Starting at all zeros means the
        # agent knows nothing: every landing looks the same.
        self.mean = np.zeros(len(omatris.RICH), np.float32)
        self.std = np.full(len(omatris.RICH), config.init_std, np.float32)
        self.generation = 0

    # -- playing --------------------------------------------------------------

    def act(self, obs, mask, explore=False) -> int:
        # The agent's best guess so far is the mean of the distribution.
        return self._choose(self.mean, obs, mask)

    def _choose(self, weights: np.ndarray, obs, mask) -> int:
        """The landing whose features, weighted, add up to the most."""
        features = omatris.rich_features(obs, self.spec, omatris.landing_count(mask)) / omatris.RICH_SCALE
        return int(np.argmax(features @ weights))

    def _fitness(self, weights: np.ndarray, env, seed: int) -> tuple[float, int]:
        """Plays one game with `weights`; how well it did, and the steps it took."""
        c = self.config
        obs = env.reset(seed)
        dealt = self.spec.tensors["stats"].column("dealt_rows_left")
        dealt_at_start = float(obs["stats"][dealt])
        steps, lines = 0, 0.0
        while True:
            step = env.step(self._choose(weights, obs, env.mask()))
            steps += 1
            lines += step.signals["lines"]
            if step.done:
                break
            obs = step.obs
        won = step.terminated and not step.signals["topped_out"]
        fitness = env.info()["score"] if c.objective == "score" else lines
        fitness += c.fitness_win * won + c.fitness_dealt_row * (dealt_at_start - float(step.obs["stats"][dealt]))
        return fitness, steps

    # -- learning -------------------------------------------------------------

    def train(self, ctx):
        c = self.config
        env = ctx.make_env(max_steps=c.episode_steps)
        elite_count = max(2, round(c.population * c.elite))
        steps = 0
        while True:
            self.generation += 1

            # Common random numbers: every candidate this generation plays the
            # same games. Then a candidate beats another because its weights
            # are better, not because it was dealt kinder pieces. (The same
            # reasoning as omagym's fixed evaluation games.)
            seeds = [ctx.next_seed() for _ in range(c.games)]

            # 1. Sample a population around the current mean.
            population = self.rng.normal(self.mean, self.std, size=(c.population, len(self.mean)))

            # 2. Score each candidate: its average over the games.
            fitness = np.zeros(c.population)
            for i, weights in enumerate(population):
                results = [self._fitness(weights.astype(np.float32), env, seed) for seed in seeds]
                fitness[i] = np.mean([score for score, _ in results])
                steps += sum(taken for _, taken in results)

            # 3. Select the elite: the best `elite_count`, best first.
            elite = population[np.argsort(-fitness)[:elite_count]]

            # 4. Refit the Gaussian to them, plus the fading noise.
            noise = c.noise * max(0.0, 1.0 - self.generation / c.noise_generations)
            self.mean = elite.mean(axis=0).astype(np.float32)
            self.std = (elite.std(axis=0) + noise).astype(np.float32)

            yield {
                "steps": steps,
                "generation": float(self.generation),
                "best_fitness": float(fitness.max()),
                "mean_fitness": float(fitness.mean()),
                "elite_fitness": float(np.sort(fitness)[-elite_count:].mean()),
                # How undecided the search still is: the average spread.
                "spread": float(self.std.mean()),
            }

    # -- keeping what was learned ---------------------------------------------

    def weights(self) -> dict[str, float]:
        """The learned weights by feature name, in scaled units: readable, and
        what `lookahead --set model=<this run>` searches with."""
        return {name: float(w) for name, w in zip(omatris.RICH, self.mean)}

    def save(self, path: Path) -> None:
        # Plain JSON rather than a binary format: twenty numbers you can read.
        state = {"weights": self.weights(), "std": self.std.tolist(), "generation": self.generation}
        path.write_text(json.dumps(state, indent=2) + "\n")

    def load(self, path: Path) -> None:
        state = json.loads(path.read_text())
        self.mean = np.array([state["weights"][name] for name in omatris.RICH], np.float32)
        self.std = np.array(state["std"], np.float32)
        self.generation = state["generation"]
