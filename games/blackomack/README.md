# Black Omack

Blackjack for Omarchy. Sit at a table with up to six AI table mates, each with
a persistent personality, and try to grow your stack of Omabucks (Ø). A fresh
table seats four of them.

## Rules (v1)

- Six-deck shoe, reshuffled when fewer than a quarter of the cards remain.
- Dealer stands on all 17s (including soft 17) and peeks for blackjack when
  showing an ace or a ten-value card.
- Blackjack pays 3:2, wins pay 1:1, pushes return the bet.
- Double down on any first two cards (one card only), including after a split.
- Split any pair, and split again up to four hands a seat; split aces get one
  card each, cannot hit and cannot be re-split.
- When the dealer shows an ace, every seat with a bet is offered insurance
  before the peek: a side bet of half the original stake that pays 2 to 1 if
  the dealer has blackjack and is lost otherwise. A seat that takes it puts a
  second, smaller "Ins" coin down beside its bet chip; at the peek the coin
  turns green or red with what it won or lost and then leaves the table. It is
  the one bet the coach tells you to refuse.
- No surrender.
- Bets from Ø 10 up to your whole bankroll, in steps of 10. `1`, `2` and `3`
  stake three presets sized to your stack — roughly a tenth, a quarter and a
  half of it, each snapped to a round 1, 2 or 5 amount (Ø 1,270 offers
  Ø 100 / Ø 200 / Ø 500; Ø 60 offers Ø 10 / Ø 20 / Ø 50) and never below the
  Ø 10 table minimum or above your bankroll. A stack too thin to keep three
  apart shows fewer buttons. Doubling and splitting require enough bankroll to
  cover the extra bet.

You start with Ø 1,000. The bankroll, your table mates and session stats are
saved between launches, as are the window's size and position. The action dock
carries the at-a-glance line — round, shoe depth, your wallet and your session
net — and the big balance under your seat repeats the wallet; every seat shows
its own cumulative net beside its bankroll.
At the payout every hand shows what it paid. Your best bankroll ever sits in the
same line and lights up for the round that sets it; it survives new games and
only a settings wipe clears it. When you're broke you can only start a new game.

## House Table

The table is arranged around a themed oval: the dealer stays at top-center,
AI table mates fill the arc symmetrically, and your larger accent-framed tray
stays bottom-center, straight across from the dealer. Play follows the seating
round the table, so the action reaches you when the sweep reaches the bottom.
Dealt cards travel from the dealer's shoe to the seat they land on, one at a
time as the deal paces them.
Bets sit beside their hands as chips, with the insurance coin next to them
when a seat has taken the side bet. The felt itself carries the last three
table events and, when there is room, the house rules line. The fixed action
dock shows only the current phase's commands together with the round, shoe
depth, your wallet, your session net, your best bankroll and the latest table
event.

Heads-up and two-mate tables enlarge the dealer and player cards without
drawing empty seats. The arc is re-spaced for the number of mates rather than
reusing fixed slots, so seats never touch; at 160% text scale, or when those
seats no longer fit at their minimum size, the table reflows into a large
dealer/player stage and a scrollable play-order roster. Identity, personality,
bankroll, bet, cards, total, active state, result and net payout remain visible
in both layouts. Split and long hands grow or wrap instead of covering seat
details.

**Small windows.** Below roughly 1040×650 the oval has no room for a third
seat, so the table caps at two mates: the header reads `n / 2` and neither `]`
nor `+` seats another one. A bigger table you saved is never evicted — its
mates move to the stage-and-roster layout, where `[` can still send them home
one at a time — and the cap lifts as soon as the window grows again. The saved
table itself is always 0–6 mates.

**Seating.** Bot seats are 190×132 logical pixels and fill the arc in play
order: the first mate sits at the dealer's left (the right of the screen), the
rest follow down that side, past your tray at the bottom and up the far side to
the dealer's right, so an odd extra mate takes the far side and the sweep ends
on it.
Up to four mates sit in two columns per side, spaced so a seat that grows with
its cards can never cover its neighbour; five and six add a third row and need
roughly a 1280×800 window, below which they reflow to the stage and scrollable
roster, where all six mates remain available in play order. The seat minimums
and the 430×181 human tray do not shrink.

## Decision Coach

The optional coach suggests the standard play for your hand against the
dealer's card: on your turn it names the play and the spot it applies to —
`Hit`, "16 against a 10" — and the rest of the time it says only `Coach on`.
It starts off, remembers your choice, and toggles with `C` or a click on the
panel; the play itself
comes from the same tested `BasicStrategy` engine the table mates use, in the
C++ bridge rather than in QML.

## Table mates

Each bot is rolled with a `skill` (how well it plays: rookie, regular, pro)
and an `aggression` (bet sizing and appetite for risk: timid, steady, bold).
Insurance is a losing bet, so pros always refuse it while rookies — the bolder
the likelier — sometimes take it.
Under its name it says how it bets and then how it plays — "timid pro",
"bold rookie", "steady regular". Play goes round the table in seating order:
bets, cards, the insurance offer and the turns all start at the mate on the
dealer's left, sweep down that side to your tray at the bottom, and carry on
up the far side to the dealer's right — so at a four-mate table two mates act
before you and two after. A bot that goes broke leaves and a fresh one takes
its seat. Change how many are at the table (0–6) between rounds with the
`−`/`+` buttons in the header; the choice is saved. A compact window caps the
table at two mates — see **Small windows** above.

## Keyboard

Everything is reachable without a mouse; each button shows its key as a badge.

| Key | Action |
|---|---|
| `Enter` / `Space` | Deal / next round; `Space` also finishes automatic pacing without playing your hand |
| `H` `S` `D` `P` | Hit / Stand / Double / Split |
| `I` / `N` | Take insurance / no insurance (only while the offer stands) |
| `↑` `↓` or `+` `-` | Bet ±10 |
| `M` | Bet max |
| `1` `2` `3` | Bet the small / medium / large preset for your bankroll |
| `B` | Type a bet (`Enter` deals, `Escape` reverts) — bet controls only exist while betting |
| `[` `]` | Fewer / more table mates (between rounds) |
| `C` | Toggle the Decision Coach (off by default) |
| `Ctrl+N` | New game, between rounds (asks for confirmation) |
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
