#pragma once

#include <stdint.h>

#define GRID_SIZE       9
#define COLOR_COUNT     7
#define PREVIEW_COUNT   3

/*
 * Game board state.
 *
 * cells[y][x] is 0 for an empty cell, or 1..7 for a ball color.
 */
typedef struct {
    uint8_t cells[GRID_SIZE][GRID_SIZE];
    uint8_t preview[PREVIEW_COUNT];
    uint16_t score;
    uint8_t game_over;
} game_t;

/* Start a new game, including the initial three balls. */
void game_init(game_t* game);

/* True when the cell contains a ball. */
uint8_t game_has_ball(const game_t* game, uint8_t x, uint8_t y);

/* True when a valid orthogonal path exists from the ball to an empty cell. */
uint8_t game_find_path(const game_t* game, uint8_t sx, uint8_t sy,
                       uint8_t tx, uint8_t ty);

/*
 * Build a path from the ball to an empty cell.
 *
 * path receives cell indices (y * GRID_SIZE + x) in move order, excluding
 * the source and including the target. It must hold at least GRID_SIZE *
 * GRID_SIZE bytes. Returns 1 when the path exists. Either output may be NULL.
 */
uint8_t game_path(const game_t* game, uint8_t sx, uint8_t sy,
                  uint8_t tx, uint8_t ty,
                  uint8_t* path, uint8_t* path_len);

/*
 * Mark every ball that would be removed by a figure containing (x, y).
 *
 * mark must hold GRID_SIZE * GRID_SIZE bytes. Returns the number of marked
 * cells. The board is not modified.
 */
uint8_t game_match_mark(const game_t* game, uint8_t x, uint8_t y,
                        uint8_t* mark);

/*
 * Simulate moving a ball from (sx, sy) to (tx, ty) and mark the resulting
 * figures. Used for match pre-highlighting.
 */
uint8_t game_match_preview(const game_t* game, uint8_t sx, uint8_t sy,
                           uint8_t color, uint8_t tx, uint8_t ty,
                           uint8_t* mark);

/* Remove marked cells and add score. Returns the removed count. */
uint8_t game_remove_mark(game_t* game, const uint8_t* mark);

/* Move a ball without checking matches or spawning. */
void game_move_ball(game_t* game, uint8_t sx, uint8_t sy,
                    uint8_t tx, uint8_t ty);

/* Place up to three preview balls without resolving matches. */
void game_place_preview(game_t* game,
                        uint8_t* spawned_x, uint8_t* spawned_y,
                        uint8_t* spawned_count);

/*
 * Move a ball and resolve the turn.
 *
 * Returns 0 when the move is invalid, 1 when the ball was moved.
 * The caller must inspect game->score and game->game_over afterwards.
 */
uint8_t game_try_move(game_t* game, uint8_t sx, uint8_t sy,
                      uint8_t tx, uint8_t ty);

/*
 * Mark and remove all lines and crosses that contain (x, y).
 *
 * Returns the number of removed balls, or 0 when no figure is complete.
 */
uint8_t game_resolve_matches(game_t* game, uint8_t x, uint8_t y);

/* Add up to three preview balls and generate their replacements. */
void game_spawn_preview(game_t* game);

/* True when all 81 cells contain a ball. */
uint8_t game_board_full(const game_t* game);

/* True when at least one ball is on the board. */
uint8_t game_has_balls(const game_t* game);

/* Generate one random color in the range 1..7. */
uint8_t game_random_color(void);
