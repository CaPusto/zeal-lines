#pragma once

#include "game.h"

/*
 * Show the "GAME OVER" popup over the board, then the high-score table.
 * The score is inserted into the table (and saved) before the table is drawn.
 *
 * Returns 1 when the player wants a new game, 0 to exit to Zeal OS. The video
 * mode stays initialized in both cases.
 */
uint8_t gameover_run(game_t* game);
