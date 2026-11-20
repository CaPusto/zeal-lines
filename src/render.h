#pragma once

#include <stdint.h>

#include <zvb_gfx.h>

#include "game.h"

#define GRID_CELLS (GRID_SIZE * GRID_SIZE)

/* Optional highlights drawn over the board. */
typedef struct {
    uint8_t path[GRID_CELLS];    /* reachable path cells */
    uint8_t preview[GRID_CELLS]; /* cells that would be removed */
    uint8_t blink;               /* preview blink phase */
    uint8_t pulse;               /* selected ball size phase */
} render_overlay_t;

/* Board placement and geometry in 16x16 tiles. */
#define GRID_TILE_X     1
#define GRID_TILE_Y     1
#define CELL_TILES      3

/* HUD panel placement in 16x16 tiles. */
#define PANEL_TILE_X    29
#define PANEL_TILE_Y    1
#define PANEL_TILE_W    10
#define PANEL_TILE_H    27

#define SCREEN_TILES_X  40
#define SCREEN_TILES_Y  30

/* Initialize the Video Board, load the palette and the first tileset frame.
 * Returns 0 on success. */
int render_init(void);

/* Two halves of render_init(), for callers that need the video board before
 * the game assets (the splash screen). */
int render_video_init(void);
int render_assets_init(void);

/* The initialized Video Board context. */
gfx_context* render_context(void);

/* Initialize the sprite HUD and draw its static English text. */
void render_hud_init(const game_t* game);

/* Update the score and next-ball preview in the sprite HUD. */
void render_hud_update(const game_t* game);

/* Restore the Zeal OS text console. */
void render_shutdown(void);

/* Redraw the whole visible screen. */
void render_board(const game_t* game,
                  uint8_t cursor_x, uint8_t cursor_y,
                  uint8_t selected, uint8_t sel_x, uint8_t sel_y,
                  const render_overlay_t* overlay);

/* Redraw one board cell, for cursor updates and small changes. */
void render_cell(const game_t* game,
                 uint8_t cursor_x, uint8_t cursor_y,
                 uint8_t selected, uint8_t sel_x, uint8_t sel_y,
                 const render_overlay_t* overlay,
                 uint8_t cx, uint8_t cy);

/* Redraw one cell with an explicit ball size, for animations. */
void render_cell_size(const game_t* game, uint8_t cx, uint8_t cy,
                      uint8_t size, uint8_t highlighted);

/* Wait for one video frame. */
void render_wait_frame(void);

/* Redraw only the cells that changed since the previous frame. */
void render_update(const game_t* game,
                   uint8_t cursor_x, uint8_t cursor_y,
                   uint8_t selected, uint8_t sel_x, uint8_t sel_y,
                   const render_overlay_t* overlay);

/* Force the next render_update() to repaint the whole screen. */
void render_invalidate(void);

/* True when the given screen pixel is inside the 9x9 board. */
uint8_t render_board_hit(uint16_t x, uint16_t y);

/* True when the given screen pixel is inside the HUD "NEW GAME" button. */
uint8_t render_hud_button_hit(uint16_t x, uint16_t y);

/* Move every HUD sprite, including the cursor reticle, off screen. */
void render_hud_hide(void);

/* Fill the whole 40x30 tilemap with one solid tile. */
void render_screen_fill(uint16_t tile);

/* Fill a tile rectangle (tile coordinates). */
void render_box(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t tile);

/* Modal screens reuse the HUD sprites for transparent text over the tilemap.
 * begin() hides every sprite and resets the text cursor. */
void render_sprites_begin(void);

/* Draw a line of uppercase ASCII text at screen pixels (8 px per character). */
void render_sprites_text(int16_t x, int16_t y, const char* text);

/* Same, with explicit character spacing. */
void render_sprites_text_spaced(int16_t x, int16_t y, const char* text,
                                uint8_t spacing);

/* Same, with a palette group 0..15 (0 is the white HUD font). */
void render_sprites_text_color(int16_t x, int16_t y, const char* text,
                               uint8_t spacing, uint8_t palette);

/* Draw one atlas tile as a sprite, for decorative balls and frames. */
void render_sprites_tile(int16_t x, int16_t y, uint16_t tile,
                         uint8_t palette);
