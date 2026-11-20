# Random number generator

The game uses the ZGDK RNG (`<zgdk/utils.h>`), but its seed is collected from
Zeal 8-bit Computer hardware sources.

## ZGDK `rand8()`

- `rand8()` is a 16-bit LFSR; every call also mixes in the Z80 `R` (refresh)
  register.
- `rand8_seed(seed)` sets the initial value; `0` is ignored.
- `rand8_quick()` returns `R` directly, without the LFSR.
- Even with a fixed seed the sequence is not guaranteed: `R` depends on the
  instructions executed and on timing.

## Seed with hardware entropy

`src/entropy.c` (`entropy_seed_random()`):

1. Reads the date and time from the DS1307 RTC through `getdate()`:
   seconds, minutes, hours, day, month, year.
2. Reads the millisecond timer through `gettime(0, ...)` when the target
   supports it.
3. Takes **16 Z80 `R` samples** through `rand8_quick()`.
4. Mixes everything into a non-zero 16-bit seed:
   XOR, rotate left by 5, add `0x9E37`.
5. Passes the result to `rand8_seed()`.

If the RTC or `gettime()` is unavailable, the `R` samples and the initial value
`0x5EED` remain — the game still starts.

There is no true TRNG on Zeal; RTC + timer + `R` is the available hardware
entropy, sufficient for choosing colours and ball positions. It is not intended
for cryptography.

## Uniform choice

To avoid modulo bias:

- colour: values `252..255` are rejected, then `value % 7`;
- empty cell: values above `256 - (256 % empty)` are rejected, then
  `pick % empty`.

## Files

- `src/entropy.c`, `src/entropy.h` — seed collection;
- `src/game.c` — `game_random_color()` and `game_place_preview()`.
