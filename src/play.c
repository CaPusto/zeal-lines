#include <stdint.h>
#include <string.h>

#include <zgdk.h>

#include "play.h"
#include "input.h"
#include "save.h"
#include "sfx.h"

#define CURSOR_START       4
#define MOVE_STEP_FRAMES   3
#define CLEAR_STEP_FRAMES  5
#define SPAWN_STEP_FRAMES  5
#define PULSE_FRAMES       30
#define BLINK_FRAMES       16

#define CELL_PIXELS        (CELL_TILES * 16)
#define GRID_PIXEL_X       (GRID_TILE_X * 16)
#define GRID_PIXEL_Y       (GRID_TILE_Y * 16)
#define SCREEN_PIXELS_X    (SCREEN_TILES_X * 16)
#define SCREEN_PIXELS_Y    (SCREEN_TILES_Y * 16)


static void play_after_move(play_t* play);
static void play_after_spawn(play_t* play);


static void play_refresh(play_t* play)
{
    uint8_t path[GRID_SIZE * GRID_SIZE];
    uint8_t mark[GRID_SIZE * GRID_SIZE];
    uint8_t len;
    uint8_t count;
    uint8_t color;
    uint8_t i;

    memset(&play->overlay, 0, sizeof(play->overlay));
    play->preview_count = 0;

    if (play->selected &&
        game_has_ball(&play->game, play->sel_x, play->sel_y) &&
        !game_has_ball(&play->game, play->cursor_x, play->cursor_y)) {

        if (game_path(&play->game, play->sel_x, play->sel_y,
                      play->cursor_x, play->cursor_y, path, &len)) {
            uint8_t cursor_index = (uint8_t)(play->cursor_y * GRID_SIZE +
                                              play->cursor_x);
            for (i = 0; i < len; i++) {
                if (path[i] != cursor_index)
                    play->overlay.path[path[i]] = 1;
            }

            color = play->game.cells[play->sel_y][play->sel_x];
            count = game_match_preview(&play->game,
                                       play->sel_x, play->sel_y, color,
                                       play->cursor_x, play->cursor_y, mark);

            if (count >= 5) {
                memcpy(play->overlay.preview, mark, sizeof(mark));
                play->preview_count = count;
            }
        }
    }

    render_update(&play->game,
                  play->cursor_x, play->cursor_y,
                  play->selected, play->sel_x, play->sel_y,
                  &play->overlay);
}


void play_reset(play_t* play)
{
    play->cursor_x = CURSOR_START;
    play->cursor_y = CURSOR_START;
    play->selected = 0;
    play->sel_x = 0;
    play->sel_y = 0;
    play->state = PLAY_IDLE;
    play->frame = 0;
    play->path_len = 0;
    play->path_step = 0;
    play->anim_size = 0;
    play->anim_tick = 0;
    play->clear_count = 0;
    play->spawned_count = 0;
    play->preview_count = 0;
    memset(&play->overlay, 0, sizeof(play->overlay));

    play_mouse_recenter(play);
    play_refresh(play);
}


void play_mouse_recenter(play_t* play)
{
    play->mouse_x = (int16_t)(GRID_PIXEL_X +
                              play->cursor_x * CELL_PIXELS + CELL_PIXELS / 2);
    play->mouse_y = (int16_t)(GRID_PIXEL_Y +
                              play->cursor_y * CELL_PIXELS + CELL_PIXELS / 2);
}


void play_mouse_track(play_t* play, int16_t dx, int16_t dy)
{
    int16_t x = (int16_t)(play->mouse_x + dx);
    int16_t y = (int16_t)(play->mouse_y + dy);

    if (x < 0)
        x = 0;
    else if (x >= SCREEN_PIXELS_X)
        x = SCREEN_PIXELS_X - 1;

    if (y < 0)
        y = 0;
    else if (y >= SCREEN_PIXELS_Y)
        y = SCREEN_PIXELS_Y - 1;

    play->mouse_x = x;
    play->mouse_y = y;
}


uint8_t play_mouse_apply(play_t* play)
{
    uint8_t cx;
    uint8_t cy;

    if (!render_board_hit((uint16_t)play->mouse_x, (uint16_t)play->mouse_y))
        return 0;

    cx = (uint8_t)((play->mouse_x - GRID_PIXEL_X) / CELL_PIXELS);
    cy = (uint8_t)((play->mouse_y - GRID_PIXEL_Y) / CELL_PIXELS);

    if (cx >= GRID_SIZE)
        cx = GRID_SIZE - 1;
    if (cy >= GRID_SIZE)
        cy = GRID_SIZE - 1;

    if (cx != play->cursor_x || cy != play->cursor_y) {
        play->cursor_x = cx;
        play->cursor_y = cy;
        play_refresh(play);
    }

    return 1;
}


void play_new_game(play_t* play)
{
    game_init(&play->game);
    save_store(&play->game);
    zsfx_new_game();
    play_reset(play);
    render_hud_update(&play->game);
    game_input_clear();
}


static void play_spawn_preview(play_t* play)
{
    uint8_t i;

    game_place_preview(&play->game,
                       play->spawned_x, play->spawned_y,
                       &play->spawned_count);

    render_hud_update(&play->game);

    if (play->spawned_count == 0) {
        play->state = PLAY_IDLE;
        play->game.game_over = game_board_full(&play->game);

        if (play->game.game_over)
            save_clear();
        else
            save_store(&play->game);

        play_refresh(play);
        return;
    }

    zsfx_spawn();
    play->state = PLAY_SPAWNING;
    play->anim_size = 6;
    play->anim_tick = 0;
    play->selected = 0;
    memset(&play->overlay, 0, sizeof(play->overlay));

    for (i = 0; i < play->spawned_count; i++) {
        if (play->game.cells[play->spawned_y[i]][play->spawned_x[i]] != 0) {
            render_cell_size(&play->game,
                             play->spawned_x[i], play->spawned_y[i],
                             6, 0);
        }
    }
}


static void play_after_move(play_t* play)
{
    uint8_t count;

    count = game_match_mark(&play->game, play->move_x, play->move_y,
                            play->clear_mark);

    if (count >= 5) {
        zsfx_clear(count);
        play->clear_count = count;
        play->state = PLAY_CLEARING;
        play->anim_size = 0;
        play->anim_tick = 0;
        play->selected = 0;
        memset(&play->overlay, 0, sizeof(play->overlay));
        return;
    }

    play_spawn_preview(play);
}


static void play_after_spawn(play_t* play)
{
    uint8_t combined[GRID_SIZE * GRID_SIZE];
    uint8_t mark[GRID_SIZE * GRID_SIZE];
    uint8_t total;
    uint8_t count;
    uint8_t i;
    uint8_t j;

    memset(combined, 0, sizeof(combined));
    total = 0;

    for (i = 0; i < play->spawned_count; i++) {
        uint8_t sx = play->spawned_x[i];
        uint8_t sy = play->spawned_y[i];

        if (play->game.cells[sy][sx] == 0)
            continue;

        count = game_match_mark(&play->game, sx, sy, mark);

        for (j = 0; j < GRID_SIZE * GRID_SIZE; j++) {
            if (mark[j] && !combined[j]) {
                combined[j] = 1;
                total++;
            }
        }
    }

    if (total >= 5) {
        zsfx_clear(total);
        memcpy(play->clear_mark, combined, sizeof(combined));
        play->clear_count = total;
        play->state = PLAY_CLEARING;
        play->anim_size = 0;
        play->anim_tick = 0;
        play->selected = 0;
        return;
    }

    play->state = PLAY_IDLE;
    play->selected = 0;
    play->game.game_over = game_board_full(&play->game);

    if (play->game.game_over)
        save_clear();
    else
        save_store(&play->game);

    render_hud_update(&play->game);
    play_refresh(play);
}


void play_input(play_t* play, uint16_t pressed)
{
    uint8_t moved = 0;

    if (play->state != PLAY_IDLE)
        return;

    if (pressed & BUTTON_Y) {
        play_new_game(play);
        return;
    }

    /* Both axes can change in one batch when frames are slow. */
    if (pressed & BUTTON_LEFT) {
        if (play->cursor_x > 0) {
            play->cursor_x--;
            moved = 1;
        }
    } else if (pressed & BUTTON_RIGHT) {
        if (play->cursor_x + 1 < GRID_SIZE) {
            play->cursor_x++;
            moved = 1;
        }
    }

    if (pressed & BUTTON_UP) {
        if (play->cursor_y > 0) {
            play->cursor_y--;
            moved = 1;
        }
    } else if (pressed & BUTTON_DOWN) {
        if (play->cursor_y + 1 < GRID_SIZE) {
            play->cursor_y++;
            moved = 1;
        }
    }

    if (moved) {
        play_mouse_recenter(play);
        play_refresh(play);
    }

    if (pressed & BUTTON_B) {
        if (play->selected) {
            play->selected = 0;
            zsfx_select();
            play_refresh(play);
        }
        return;
    }

    if (pressed & (BUTTON_A | BUTTON_START)) {
        if (!play->selected) {
            if (game_has_ball(&play->game, play->cursor_x, play->cursor_y)) {
                play->selected = 1;
                play->sel_x = play->cursor_x;
                play->sel_y = play->cursor_y;
                zsfx_select();
                play_refresh(play);
            }
        } else if (play->cursor_x == play->sel_x &&
                   play->cursor_y == play->sel_y) {
            play->selected = 0;
            zsfx_select();
            play_refresh(play);
        } else if (game_has_ball(&play->game,
                                 play->cursor_x, play->cursor_y)) {
            play->sel_x = play->cursor_x;
            play->sel_y = play->cursor_y;
            zsfx_select();
            play_refresh(play);
        } else {
            uint8_t len;

            if (game_path(&play->game, play->sel_x, play->sel_y,
                          play->cursor_x, play->cursor_y,
                          play->path, &len)) {
                play->path_len = len;
                play->path_step = 0;
                play->move_x = play->sel_x;
                play->move_y = play->sel_y;
                play->anim_tick = 0;
                zsfx_move();
                play->selected = 1;
                play->state = PLAY_MOVING;
                play->preview_count = 0;
                memset(&play->overlay, 0, sizeof(play->overlay));
            }
        }
    }
}


void play_tick(play_t* play)
{
    uint8_t i;

    play->frame++;

    if (play->state == PLAY_IDLE) {
        if (play->selected &&
            game_has_ball(&play->game, play->sel_x, play->sel_y) &&
            (play->frame % PULSE_FRAMES) == 0) {
            play->overlay.pulse ^= 1;
            render_update(&play->game,
                          play->cursor_x, play->cursor_y,
                          play->selected, play->sel_x, play->sel_y,
                          &play->overlay);
        }

        if (play->preview_count &&
            (play->frame % BLINK_FRAMES) == 0) {
            play->overlay.blink ^= 1;
            render_update(&play->game,
                          play->cursor_x, play->cursor_y,
                          play->selected, play->sel_x, play->sel_y,
                          &play->overlay);
        }

        return;
    }

    if (play->state == PLAY_MOVING) {
        uint8_t index;
        uint8_t nx;
        uint8_t ny;
        uint8_t old_x;
        uint8_t old_y;
        uint8_t color;

        if (++play->anim_tick < MOVE_STEP_FRAMES)
            return;

        play->anim_tick = 0;

        if (play->path_step >= play->path_len) {
            play_after_move(play);
            return;
        }

        index = play->path[play->path_step];
        nx = (uint8_t)(index % GRID_SIZE);
        ny = (uint8_t)(index / GRID_SIZE);
        old_x = play->move_x;
        old_y = play->move_y;
        color = play->game.cells[old_y][old_x];

        play->game.cells[old_y][old_x] = 0;
        play->game.cells[ny][nx] = color;

        play->move_x = nx;
        play->move_y = ny;
        play->sel_x = nx;
        play->sel_y = ny;

        render_cell(&play->game,
                    play->cursor_x, play->cursor_y,
                    play->selected, play->sel_x, play->sel_y,
                    &play->overlay, old_x, old_y);
        render_cell(&play->game,
                    play->cursor_x, play->cursor_y,
                    play->selected, play->sel_x, play->sel_y,
                    &play->overlay, nx, ny);

        play->path_step++;

        if (play->path_step >= play->path_len)
            play_after_move(play);

        return;
    }

    if (play->state == PLAY_CLEARING) {
        if (++play->anim_tick < CLEAR_STEP_FRAMES)
            return;

        play->anim_tick = 0;

        if (play->anim_size < 6) {
            uint8_t x;
            uint8_t y;

            play->anim_size++;

            for (y = 0; y < GRID_SIZE; y++) {
                for (x = 0; x < GRID_SIZE; x++) {
                    if (play->clear_mark[y * GRID_SIZE + x] &&
                        play->game.cells[y][x] != 0) {
                        render_cell_size(&play->game, x, y,
                                         play->anim_size, 0);
                    }
                }
            }
            return;
        }

        game_remove_mark(&play->game, play->clear_mark);
        render_hud_update(&play->game);

        /* After a clear no new balls are added for that turn, but an empty
         * board would leave nothing to move: refill it from the preview. */
        if (!game_has_balls(&play->game)) {
            memset(&play->overlay, 0, sizeof(play->overlay));
            play_spawn_preview(play);
            return;
        }

        save_store(&play->game);

        play->state = PLAY_IDLE;
        play->selected = 0;
        play->clear_count = 0;
        memset(&play->overlay, 0, sizeof(play->overlay));
        play_refresh(play);
        return;
    }

    if (play->state == PLAY_SPAWNING) {
        if (++play->anim_tick < SPAWN_STEP_FRAMES)
            return;

        play->anim_tick = 0;

        if (play->anim_size > 0) {
            play->anim_size--;

            for (i = 0; i < play->spawned_count; i++) {
                if (play->game.cells[play->spawned_y[i]][play->spawned_x[i]] != 0) {
                    render_cell_size(&play->game,
                                     play->spawned_x[i], play->spawned_y[i],
                                     play->anim_size, 0);
                }
            }
            return;
        }

        play_after_spawn(play);
    }
}
