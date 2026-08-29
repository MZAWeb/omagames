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
- `main` stays untouched while work is in flight; only merge into it.
- One branch per worktree; branch per game or per feature.

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
- it finishes with `rm -rf build build-tests && bin/build <game> && bin/test <game>`
  and a report: done / missing / decisions / test totals / wishes outside its paths.

Tip: when driving an agent through tmux, send prompts with
`tmux send-keys -l "<text>"` followed by a separate `send-keys Enter`.

## Reviewing

```sh
git log --oneline main..<branch>
git diff main...<branch> --stat          # scope check: nothing outside the allowed paths
git diff main...<branch> -- common/      # shared changes deserve a close look
.worktrees/<branch>/bin/run <game>       # play it
.worktrees/<branch>/bin/test <game>
```

Or push and open a PR: `git push -u origin <branch>` then `gh pr create --base main`.
Ask for fixes by re-prompting the same agent in its worktree.

## Merging

```sh
git merge --no-ff <branch>
bin/build && bin/test
```

If two branches both added controls to `common/`, the `qmldir` / `common.qrc`
conflict is resolved by keeping both sides' lines. Bring a lagging branch up to
date from inside its worktree with `git merge main`.

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
