# Basilevs

**Dodge the swarm, gun down the horrors — a 1-bit bullet-hell shmup for Playdate.**

Basilevs is a vertical bullet-hell shoot-'em-up on the Playdate's crisp 1-bit
screen. You pilot a lone gunship at the bottom of a scrolling battlefield while
grasping tentacles rain aimed darts and mosquitoes bloom whole rings of curving
shot around you. There is no cover and no bomb — only the gaps between the
bullets. Weave through, shoot everything down, and keep your HP bar off the
floor.

This is an unofficial Playdate port of the original **basilevs** by **bredlej**
(a.k.a. geoco): <https://github.com/bredlej/basilevs>. All of the game's design,
art, sound, and level timeline are bredlej's work — this build re-implements
them in C for the Playdate hardware. Every enemy dies in one hit, so the
challenge is never toughness but survival: read the patterns, thread the gaps,
and learn the wave order.

## Features

- Twitchy 1-bit bullet-hell action native to the Playdate's 400×240 screen
- Two enemy types with distinct patterns: **tentacles** fire heavy aimed darts
  you must dodge; **mosquitoes** bloom 12-bullet rings you can shoot down
- Bullets that **curve and accelerate** — mosquito rings warp into spirals if
  you let them linger
- One-hit enemies and a fixed, learnable wave timeline
- HP bar, live HITS score, title screen, and instant retry
- Original chiptune-style music and art by bredlej

## Controls

- **D-pad** — move (eight directions)
- **A** (hold) — fire a four-way spread, auto-repeating
- **B** — restart the run
- **A** to start on the title screen; **A**/**B** to retry after a game over

## Install (no dev tools needed)

1. Download **Basilevs.pdx.zip** from the Releases page (or the `dist/`
   folder).
2. Sideload it to your Playdate at <https://play.date/account/sideload/>, or
   unzip it and drop the `.pdx` into the Playdate Simulator.

That's it — no SDK or build toolchain required to play.

---

*Original game © bredlej — <https://github.com/bredlej/basilevs>. Playdate port
MIT-licensed with attribution; see the repository LICENSE.*
