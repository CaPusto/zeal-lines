# Project scope and confirmed decisions

This document records the decisions made for the Zeal version of L2 Reborn.
The GDD remains the product description; this file wins on any conflict.

## Platform and input

- Target machine: **Zeal 8-bit Computer + Zeal Video Board**.
- Mode: **640×480, 4bpp**, one game scene.
- The game is **single-player**.
- Input: **keyboard, SNES pad and SNES mouse** through ZGDK.
- The mouse can be plugged into either pad port and is detected automatically.
  It never disables the keyboard or the pad: mouse movement moves the cell
  cursor, the left button acts as `A`, the right button as `B`, and clicking the
  `NEW GAME` panel button starts a new game. Keyboard or pad cursor movement
  re-centres the mouse pointer, so the input methods can be mixed freely.
- Web and mobile versions are out of scope for this project.

## Interface and assets

- HUD in **English only** (`SCORE`, `NEXT`, `GAME OVER`, `HIGH SCORES`).
- Game title in the HUD: **`ZEAL LINES`**.
- Screen layout follows `sample-game-field.png`: a 9×9 board on the left and a
  panel on the right; all in-game typography is English.
- Preview: **separate 16×16 mini-balls**, one per colour.
- Score: **16×16 digits**, one digit per tile.
- Board ball: **32×32 px**; cell art follows current L2 (rounded cells with a
  dotted background).
- Autosave is labelled **`AUTOSAVE`** in the HUD.
- **Title screen**: a separate disk-loaded splash tileset. It is not part of the
  512-tile game atlas; afterwards VRAM is filled with the game tileset.
- **Highlighting** (cursor, selected ball, path, preview) only brightens the
  cell background/frame through a palette group. Ball shades never change.
- Detailed asset, palette and tile-budget layout: [`ASSETS.md`](ASSETS.md).

## Controls (current implementation)

| Action | Keyboard | SNES pad | Mouse |
|---|---|---|---|
| Cursor | Arrow keys | D-pad | Move the mouse |
| Select / place / confirm | **X** | A | Left button |
| New game | **A** | Y | Click `NEW GAME` |
| Cancel selection | Space / Z | B | Right button |
| Exit to Zeal OS | Right Shift / `'` | Select | — |

Enter/Start is also wired as confirm, but the Native Emulator's Return key does
not activate it; use X/A there.

## Implemented

- playable core: moves, BFS pathfinding, lines, `+`, `X`, scoring, game over;
- HUD: `ZEAL LINES`, `RETRO EDITION`, `SCORE`, `NEXT`, goal text, `AUTOSAVE`;
- 16×16 score and 16×16 preview;
- clickable `NEW GAME` button and the A/Y shortcut;
- autosave to `zeal-lines.sav` (H:/current directory/T:), restored on startup,
  cleared after game over;
- 512-tile atlas: 32-px balls, palette groups, English font;
- animations: spawn 6→0, clear 0→6, selected-ball pulse;
- path highlight and blinking match pre-highlight;
- step-by-step ball movement;
- SFX on four ZVB voices (click, move, spawn, clear, game over);
- title splash with fade in/out;
- large `ZEAL LINES` HUD title;
- `GAME OVER` popup and a persistent top-five high-score table.

All planned screens are implemented. No settings screen is needed: sound is
always on.

## Sound

- **SFX only**, no music.
- Four ZVB voices are used.
- No sound settings are required; sound is always enabled.
- A music track, tracker and music engine are out of scope.
- Implementation: [`SOUND.md`](SOUND.md).

## Technology

- Language: C (SDCC), Zeal 8-bit OS.
- Graphics: ZVB SDK, 4bpp tilemap, 16 palette groups of 16 colours.
- Input: ZGDK.
- RNG: ZGDK LFSR seeded from the RTC, the timer and 16 Z80 `R` samples
  ([`RANDOM.md`](RANDOM.md)).
- Build: **ZDE**, command `~/zeal-dev-environment/zde cmake`.
- Emulator: **Zeal Native Emulator** in `~/zeal-native-emulator`.
- Save game and high scores: files on the ZOS disk (`H:` in the emulator,
  `T:`/current directory on hardware), not LocalStorage.

## Game rules

Match conditions, scoring, spawning and the lose condition are specified in
[`MATCH_RULES.md`](MATCH_RULES.md).

Summary:

- lines of 5+ in four directions;
- full 5-ball `+` and `X` crosses;
- figures are merged without duplicate cells;
- 2 points per unique ball;
- after a clear no new balls are added;
- after a move without a clear, up to 3 preview balls are added;
- the game is over when no empty cells remain after spawning.

## Out of scope

- music;
- web version;
- mobile ports;
- networked play;
- level editor.
