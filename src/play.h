#pragma once

#include <stdint.h>

#include "game.h"
#include "render.h"

typedef enum {
    PLAY_IDLE = 0,
    PLAY_MOVING,
    PLAY_CLEARING,
    PLAY_SPAWNING
} play_state_t;

typedef struct {
    game_t game;

    uint8_t cursor_x;
    uint8_t cursor_y;
    uint8_t selected;
    uint8_t sel_x;
    uint8_t sel_y;

    render_overlay_t overlay;
    uint8_t preview_count;

    uint8_t state;
    uint16_t frame;

    uint8_t path[GRID_SIZE * GRID_SIZE];
    uint8_t path_len;
    uint8_t path_step;
    uint8_t move_x;
    uint8_t move_y;

    uint8_t anim_size;
    uint8_t anim_tick;

    uint8_t clear_mark[GRID_SIZE * GRID_SIZE];
    uint8_t clear_count;

    uint8_t spawned_x[PREVIEW_COUNT];
    uint8_t spawned_y[PREVIEW_COUNT];
    uint8_t spawned_count;

    /* Mouse pointer position in screen pixels. Keyboard and pad moves keep it
     * centred on the cursor, so both input methods can be mixed freely. */
    int16_t mouse_x;
    int16_t mouse_y;
} play_t;

/* Reset cursor, selection, overlays and animation state for the current game. */
void play_reset(play_t* play);

/* Start a fresh game, reset the play state and overwrite the save. */
void play_new_game(play_t* play);

/* Handle a pressed-button mask while the game is idle. */
void play_input(play_t* play, uint16_t pressed);

/* Advance animations and idle effects by one video frame. */
void play_tick(play_t* play);

/* Move the mouse pointer by the reported delta (screen pixels). */
void play_mouse_track(play_t* play, int16_t dx, int16_t dy);

/* Move the cursor to the cell under the mouse pointer. Returns 1 when the
 * pointer is over the board, 0 when it is over the HUD or outside it. */
uint8_t play_mouse_apply(play_t* play);

/* Centre the mouse pointer on the current cursor cell. */
void play_mouse_recenter(play_t* play);
