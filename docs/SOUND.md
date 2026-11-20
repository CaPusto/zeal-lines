# Sound (SFX)

Four Zeal Video Board voices are used. There is no music and no music engine.

## Voices

| Voice | Purpose |
|---|---|
| VOICE0 | selection click, movement tick |
| VOICE1 | shrink / clear |
| VOICE2 | ball spawn, new game |
| VOICE3 | game over |

## Effects

- **Select** — 1500 Hz, square 12.5%, 2 frames.
- **Move** — 900 Hz, square 12.5%, 2 frames.
- **Spawn** — 440 Hz → 880 Hz, square 25%.
- **Clear** — triangle:
  - 1047 Hz for 5–6 balls;
  - 1568 Hz for more than 6 balls (higher pitch);
  - then 2093 Hz and decay.
- **Game over** — 330 → 294 → 262 Hz, triangle.
- **New game** — 523 → 659 → 784 Hz.

## Implementation

`src/sfx.c`, `src/sfx.h`:

- `zsfx_init()` — initialize ZVB sound, volume and hold;
- `zsfx_update()` — per-frame timer and melody update;
- `zsfx_select()`, `zsfx_move()`, `zsfx_spawn()`, `zsfx_clear(count)`,
  `zsfx_game_over()`, `zsfx_new_game()`;
- `zsfx_stop_all()` — before exit/restart.

`zsfx_update()` is called every frame in the game loop and on the game-over
screens. Functions are prefixed `zsfx_*` to avoid a collision with ZGDK's
`sfx_init()`.

CMake links `zvb_sound`.
