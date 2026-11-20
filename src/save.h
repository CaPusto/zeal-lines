#pragma once

#include <stdint.h>

#include "game.h"

/* Load a saved game. Returns 1 when a valid save was restored. */
uint8_t save_load(game_t* game);

/* Store the current game state. Errors are ignored. */
void save_store(const game_t* game);

/* Remove saved state, for example after game over. */
void save_clear(void);
