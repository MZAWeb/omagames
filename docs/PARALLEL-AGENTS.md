# Working in parallel with git worktrees

When several people or coding agents work on different games at once, give
each one a branch and a worktree. Worktrees share `.git` (branches, history)
but have separate files and a separate untracked `build/`, so one broken
build never affects another.

## Setup

```sh
cd ~/code/omagames
git worktree add -b <branch> .worktrees/<branch> main
git worktree list
```

- Worktrees live in `.worktrees/` (gitignored) inside the main checkout.
- `main` stays untouched while work is in flight; changes reach it through PRs.
- One branch per worktree; branch per game or per feature.
- If an agent has to branch by itself inside its worktree, it must use
  `git checkout -b <branch> --no-track origin/main`. A branch tracking
  `origin/main` plus `git push -u` fast-forwards `main` (this happened once);
  the repo sets `push.default=current` as a second guard.

## Briefing an agent

Start the agent **inside its worktree**. A good prompt names the worktree and
branch, points at `CLAUDE.md`, `docs/ARCHITECTURE.md` and the game's
`README.md` (or `docs/NEW-GAME.md` for a new game), and states the deliverable.
Be explicit that:

- it stays in `games/<game>/**` (plus append-only additions to
  `common/qml/OmaGames/qmldir` and `common/common.qrc` for generic controls),
- it makes atomic commits on its branch, every commit building and passing
  `bin/test <game>`,
- it never touches `main`, never runs `git worktree`, never force-pushes,
  never merges its own PR,
- it verifies with tests, `qmllint` (the exact command from
  `.github/workflows/ci.yml`) and a run under `QT_FORCE_STDERR_LOGGING=1`
  (Qt logs QML warnings to journald when stderr is not a tty) — and does
  **not** take or update screenshots; those are refreshed by hand now and then,
- it finishes with `rm -rf build build-tests && bin/build <game> && bin/test <game>`,
  pushes, opens the PR with `gh pr create`, waits for `gh pr checks`, and reports:
  done / missing / decisions / test totals / wishes outside its paths.

The same brief works for non-Claude agents, e.g.
`codex -m <model> -c model_reasoning_effort=high -s workspace-write --add-dir ~/code/omagames/.git`.
The `--add-dir` matters: a worktree's git metadata lives in the main repo's
`.git/worktrees/<name>/`, outside Codex's writable workspace, so without it
`git commit` fails on `index.lock` and you end up committing on its behalf.

Tip: when driving an agent through tmux, send prompts with
`tmux send-keys -l "<text>"` followed by a separate `send-keys Enter` (Codex
needs the Enter twice for a multi-line paste), and use `/model` and `/effort`
in the agent's own session to switch models mid-task.

## Reviewing

```sh
git log --oneline main..<branch>
git diff main...<branch> --stat          # scope check: nothing outside the allowed paths
git diff main...<branch> -- common/      # shared changes deserve a close look
.worktrees/<branch>/bin/run <game>       # play it
.worktrees/<branch>/bin/test <game>
```

The normal path is a PR: `git push -u origin <branch>` then `gh pr create --base main`;
review the diff there, let CI run, ask for fixes by re-prompting the same agent
in its worktree (it pushes to the same branch), and merge on GitHub.

## Merging

Merge the PR on GitHub once checks are green. When two in-flight branches both
touch `common/`, the `qmldir` / `common.qrc` conflict is resolved by keeping
both sides' lines; bring a lagging branch up to date from inside its worktree
with `git fetch origin && git merge origin/main`, then rebuild, retest and run
the game once — a rename that merges cleanly can still fail at runtime.

## Cleanup

```sh
git worktree remove .worktrees/<branch>
git branch -d <branch>
git worktree prune
```

## Gotchas

- `build/` and `build-tests/` are per worktree; the first build in a fresh one is a full build.
- Games write settings to `~/.config/Omacom/<game>.conf`, shared across worktrees.
- `git stash` is per repository, not per worktree.
