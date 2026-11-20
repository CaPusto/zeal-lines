# zeal-lines

A Lines / Color Lines remake for the [Zeal 8-bit Computer](https://zeal8bit.com/),
built with the [Zeal Development Environment](https://github.com/zoul0813/zeal-dev-environment)
and the [Zeal Video Board SDK](https://github.com/Zeal8bit/Zeal-VideoBoard-SDK).

The game targets Zeal 8-bit OS and the Zeal Video Board in 640×480 4bpp mode.
It is a single-player game controlled by keyboard, SNES pad or SNES mouse.

The scope is single-player with keyboard, gamepad and mouse input, and SFX only
(no music). The input layer is kept separate from the game rules; the mouse adds
its own actions on top of the keyboard and pad.

## Current state

Implemented:

- 9×9 board, 7 colors, 32×32 px balls;
- three-ball preview and spawn;
- orthogonal pathfinding;
- line matches, direct “+” crosses and diagonal “X” crosses;
- free move after a match;
- score, game over popup, high-score table and restart;
- English sprite HUD: `ZEAL LINES`, `SCORE`, `NEXT`, goal, `AUTOSAVE`, `NEW GAME`;
- autosave to `zeal-lines.sav`, restored on startup;
- title splash from `splash.zts`/`splash.ztp`/`splash.ztm` with fade in/out;
- large `ZEAL LINES` title in the HUD;
- 512-tile 4bpp atlas with palette-based selection;
- white corner reticle around the cursor, readable on a full board;
- mouse control (SNES mouse on either pad port) next to keyboard and gamepad;
- event-based input: quick taps survive slow frames and stale direction
  repeats cannot build up a backlog;
- spawn/shrink animations, selected-ball pulse, step-by-step movement;
- path highlight and blinking match pre-highlight;
- SFX on the four Video Board voices (clear tone rises above six balls).

There is no settings screen: sound is always on, as requested.

## Build

Build through ZDE from the project directory:

```sh
cd ~/zeal-projects/zeal-lines
~/zeal-dev-environment/zde cmake
```

ZDE is expected at `~/zeal-dev-environment`. The build outputs:

- `bin/zeal-lines.bin` — raw Zeal 8-bit OS binary;
- `bin/zeal-lines.ihx`, `bin/zeal-lines.map` — intermediates;
- `bin/zeal-tiles.dat` — runtime 4bpp tileset (512 tiles, 65,536 bytes),
  copied from `assets/zeal-tiles.dat` by the build.

The game reads the tileset from `H:/zeal-tiles.dat` first and falls back to
`zeal-tiles.dat`, `T:/zeal-tiles.dat` and `A:/zeal-tiles.dat`. The tileset is
not embedded in `zeal-lines.bin`; keep it next to the executable on the SD card
or in the emulator's HostFS directory.

## Tests

The pure game logic can be tested on the host without the Zeal toolchain:

```sh
./tests/run_tests.sh
```

The tests cover lines, direct crosses, diagonal crosses, duplicate-free scoring,
pathfinding, moves, preview spawning and the full-board game-over condition. A
ball counts once per move even when several lines cross it.

## Run in the emulator

```sh
./run_emu.sh
```

The script uses the emulator at `~/zeal-native-emulator` and its
`zos/os_with_romdisk.img` ROM. Override the paths with `ZEAL_NATIVE_DIR` and
`ZEAL_ROM` if needed.

Extra emulator arguments are passed through, for example:

```sh
./run_emu.sh --headless --console
```

## Controls

| Action | Keyboard | SNES pad | Mouse |
| --- | --- | --- | --- |
| Move cursor | Arrow keys | D-pad | Move the mouse |
| Select / move / confirm | **X** | A | Left button |
| New game | **A** | Y | Click `NEW GAME` |
| Cancel selection | Space or Z | B | Right button |
| Exit to Zeal OS | Right Shift or `'` | Select | — |
| Game over / scores | `A` new game, `SELECT` exit | A / Select | Left click |

`Enter`/Start is also wired as confirm in the code, but the Native Emulator's
Return key does not activate it; use `X`/A in the emulator.

A SNES mouse is detected automatically on either controller port; it is purely
additive and never disables the keyboard or the gamepad. The mouse moves the
board cursor, the left button works like `A` and the right button like `B`.
Clicking the `NEW GAME` panel button starts a fresh game. In the Native
Emulator the host mouse is the SNES mouse; it is on controller port 2 by
default and must be inside the emulator window to be read.

Audio scope is SFX only: no music track and no music engine.

## High scores

The top five scores are kept in `zeal-scores.dat` (same lookup order as the
save: `H:`, current directory, `T:`). At game over the score is inserted,
saved, and shown on the table. On the `GAME OVER` popup any key or click
continues; on the table `A`/Start/B/click starts a new game and `SELECT` exits.

## Sound

SFX only, no music. Four ZVB voices handle selection, movement, spawning,
clearing and game over. Clearing more than six balls raises the tone.

## Randomness

The ZGDK LFSR is seeded from the DS1307 RTC date/time, the millisecond timer
when available, and 16 samples of the Z80 refresh register `R`. The values are
mixed into a non-zero 16-bit seed; color and empty-cell selection use rejection
sampling to avoid modulo bias.

## Autosave

After each turn the game writes `zeal-lines.sav` (tried in `H:`, the current
directory, then `T:`). The save is restored on startup. `A` starts a new game
and overwrites the save; game over clears it. Delete the file to force a fresh
start.

## Title screen

On startup the game shows the ready-made splash from
`assets/splash.ztp` (palette), `assets/splash.zts` (tileset) and
`assets/splash.ztm` (40×30 tilemap, layer 0 plus attribute layer 1), copied next
to the binary by the build. It fades in from black, waits for any key or mouse
click (or 25 seconds), fades out, and then loads the game palette and tileset
again.

The files are looked up the same way as the game tileset: `H:/` first, then the
current directory, `T:/` and `A:/`. If any of them is missing, the splash is
skipped and the game starts immediately.

## Assets

The tileset is generated by `tools/prepare_assets.py` from the L2 source
sprites. The generated files are committed:

- `assets/zeal-tiles.dat`
- `src/assets_generated.c`
- `src/assets_generated.h`

The implemented 512-tile layout, palette groups and HUD/preview budget are
defined by `tools/prepare_assets.py`; the tile indices are mirrored in
`src/assets_generated.h`.

The generator needs Python 3 with Pillow and network access to the upstream L2
repository.

## Backup

Create a timestamped archive in `~/backups`:

```sh
./backup.sh
```

Build artifacts, generated binaries, `.git` and virtualenvs are excluded.
Sources, assets and configuration files are included.
