# Basilevs — Player Manual

*A 1-bit bullet-hell shmup for the Playdate. Weave through the swarm, gun down the horrors, keep the HP bar off the floor.*

Basilevs is a Playdate port of the vertical shoot-'em-up
[**basilevs**](https://github.com/bredlej/basilevs) by **bredlej** (a.k.a.
geoco), the original author. The gameplay, art, sound, and level-1 spawn
timeline all follow that upstream project; this build re-implements them in
C for the Playdate. Full credit for the game's design and assets goes to
bredlej — see the README for provenance.

## The story

Something ancient has woken over the ruined skyline, and it has brought
friends. You are a lone gunship threading the space between grasping
tentacles and swarming mosquitoes, each spitting patterns of shot that fill
the screen. There is no cover and no bomb — only the gaps between the
bullets. Survive the assault, rack up the hits.

## Controls

| Playdate | Action |
| --- | --- |
| **D-pad** | Move the ship (all eight directions) |
| **A** (hold) | Fire — a four-way spread, auto-repeating while held |
| **B** | Restart the run |

On the title screen, press **A** to begin. On the game-over screen, press
**A** or **B** to try again.

## How to play

You fly at the bottom of a tall, scrolling battlefield. Enemies drift in
from the top on set paths and open fire the moment they arrive. Your job is
to shoot them down before their bullets grind your HP to zero.

**Firing.** Holding **A** launches a fan of four bullets — two flying
straight up and two angled out to the left and right — every 0.2 seconds.
The spread means you cover a wide lane, but your heaviest fire is the two
centre shots; line those up on whatever you most want dead.

**Moving.** The ship is fast and stays clamped inside the visible
playfield, so you can hug any edge without flying off-screen. Small,
deliberate movements thread bullet gaps better than big dashes.

**The HP bar** (left gutter) starts full at 100. Every enemy bullet that
touches you drains it — some far more than others (see below). When it
empties, it's game over.

**The HITS counter** (right gutter) is your score: the number of enemies
you have destroyed this run. Shooting down enemy bullets clears them from
the screen but does **not** score.

## Enemies and their bullet patterns

Every enemy dies from a **single hit**, then plays a short death animation
and vanishes. The trick is never the enemy's toughness — it's their fire.

| Enemy | Fires | Damage | Can you shoot the bullets down? |
| --- | --- | --- | --- |
| **Tentacle** | One dart, **aimed straight at you**, once per second | **30** each — brutal | **No** — you must dodge these |
| **Mosquito** | A **rose of 12 bullets** in a full circle, once per second | **1** each | **Yes** — they can be shot out of the air |

- **Tentacle darts** are the real threat: they home in on wherever you are
  the instant they fire, fly fast, and take a third of your HP per hit.
  Three of them landing ends the run. You cannot destroy these shots — only
  dodge them, so keep moving whenever a tentacle is on screen.
- **Mosquito roses** look terrifying but each pellet only costs 1 HP, and
  every one can be shot down. Worse, the pellets **curve and accelerate**:
  every half-second each bullet veers 15° and picks up speed, so a tidy
  ring gradually warps into a chaotic spiral. Clear them early, while
  they're slow and still bunched near the mosquito.

## The level-1 assault (timeline)

The wave order is fixed, so you can learn it:

1. **~1 s** — two Tentacles slide in from the left and right corners.
2. **~5.5 s** — a single Mosquito loops in along a three-point path.
3. **~11 s** — a **row of four Tentacles** drops across the top. This is
   the danger spike: four aimed darts per second.
4. **~15.5 s** — another Mosquito arrives to weave its rose through the
   survivors.

Enemies begin shooting *during* their entrance, not after they settle, so
the pressure starts the moment each wave appears.

## Tips

1. **Dodge the tentacles, farm the mosquitoes.** Tentacle darts (30 HP)
   are un-shootable and deadly; mosquito pellets (1 HP) are shootable and
   cheap. Spend your attention dodging the former and your bullets clearing
   the latter.
2. **Kill on arrival.** Enemies fire while still gliding into position.
   Meeting a wave at the top with your spread already up can drop it before
   it lands a single shot.
3. **Hold A, don't tap it.** The fire timer only counts up while the button
   is held, so tapping keeps resetting the wind-up and throws away your
   first shot each time. Keep it pressed.
4. **Fear the four-tentacle row (~11 s).** That wave puts four aimed darts
   per second on you at once. Have a clear patch of screen picked out before
   it arrives and keep drifting through it.
5. **Sweep mosquito rings before they spiral.** The pellets accelerate and
   bend every half-second; a ring you ignore for a couple of seconds becomes
   a fast, unpredictable spiral. Hit them while they're slow.
6. **Stay low and centred.** The playfield clamps you in, so anchoring near
   the bottom-centre gives you the most room and reaction time to read
   incoming aimed shots.

---

*Basilevs is by **bredlej** — <https://github.com/bredlej/basilevs>. This is
an unofficial Playdate port. MIT-licensed; see LICENSE.*
