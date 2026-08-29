# Black Omack

Blackjack for Omarchy. Sit at a table with up to five AI table mates, each with
a persistent personality, and try to grow your stack of Omabucks (Ø).

## Rules (v1)

- Six-deck shoe, reshuffled when fewer than a quarter of the cards remain.
- Dealer stands on all 17s (including soft 17) and peeks for blackjack when
  showing an ace or a ten-value card.
- Blackjack pays 3:2, wins pay 1:1, pushes return the bet.
- Double down on any first two cards (one card only), including after a split.
- Split once per hand; split aces get one card each and cannot hit.
- No insurance, no surrender.
- Bets from Ø 10 up to your whole bankroll, in steps of 10. Doubling and
  splitting require enough bankroll to cover the extra bet.

You start with Ø 1,000. The bankroll, your table mates and session stats are
saved between launches. When you're broke you can only start a new game.

## Table mates

Each bot is rolled with a `skill` (how often it follows basic strategy) and an
`aggression` (bet sizing and appetite for risk), shown under its name as
"cautious · sharp", "wild · reckless" and so on. A bot that goes broke leaves
and a fresh one takes its seat. Change the number of bots (0–5) between rounds
with the `−`/`+` buttons in the header.

## Keyboard

| Key | Action |
|---|---|
| `Enter` / `Space` | Deal / next round |
| `H` `S` `D` `P` | Hit / Stand / Double / Split |
| `↑` `↓` or `+` `-` | Bet ±10 |
| `M` | Bet max |
| `Ctrl+N` | New game (asks for confirmation) |
| `Ctrl+Q` | Quit |
| `Escape` | Leave the bet field / close a dialog |

## Build, test, run

```sh
bin/build blackomack
bin/test blackomack
bin/run blackomack
```

Settings live in `~/.config/Omacom/blackomack.conf`.

## Install (Arch)

```sh
cd games/blackomack/pkgbuild && makepkg -si
```
