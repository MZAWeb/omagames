# Black Omack

Blackjack for Omarchy. Sit at a table with up to five AI table mates, each with
a persistent personality, and try to grow your stack of Omabucks (Ø). A fresh
table seats three of them.

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

You start with Ø 1,000. The bankroll, your table mates and session stats
(hands played and net, shown under your balance) are saved between launches.
At the payout every hand shows what it paid. When you're broke you can only
start a new game.

## Table mates

Each bot is rolled with a `skill` (how well it plays: rookie, regular, pro)
and an `aggression` (bet sizing and appetite for risk: timid, steady, bold).
Under its name it says how it bets and then how it plays — "timid pro",
"bold rookie", "steady regular". A bot that goes broke leaves and a fresh one
takes its seat. Change how many are at the table (0–5) between rounds with the
`−`/`+` buttons in the header; the choice is saved.

## Keyboard

Everything is reachable without a mouse; each button shows its key as a badge.

| Key | Action |
|---|---|
| `Enter` / `Space` | Deal / next round |
| `H` `S` `D` `P` | Hit / Stand / Double / Split |
| `↑` `↓` or `+` `-` | Bet ±10 |
| `M` | Bet max |
| `B` | Type a bet (`Enter` deals, `Escape` reverts) — bet controls only exist while betting |
| `[` `]` | Fewer / more table mates (between rounds) |
| `Ctrl+N` | New game (asks for confirmation) |
| `Y` / `Enter`, `N` / `Escape` | Confirm / cancel a dialog |
| `Ctrl+Q` | Quit |

## Build, test, run

```sh
bin/build blackomack
bin/test blackomack
bin/run blackomack
```

Settings live in `~/.config/Omacom/blackomack.conf`.

## Install (Arch)

```sh
curl -fsSL https://raw.githubusercontent.com/MZAWeb/omagames/main/install.sh | bash -s blackomack
```

or from a checkout, `bin/install blackomack`.
