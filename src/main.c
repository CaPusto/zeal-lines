#include <stdint.h>
#include <stdio.h>

#include <zos_errors.h>
#include <zos_vfs.h>
#include <zos_time.h>

#include <zgdk.h>

#include "entropy.h"
#include "game.h"
#include "gameover.h"
#include "input.h"
#include "play.h"
#include "render.h"
#include "sfx.h"
#include "save.h"
#include "scores.h"
#include "splash.h"


int main(void)
{
    static play_t play;

    uint16_t events;
    uint16_t pending;
    game_mouse_t mouse;

    zos_err_t err;

    err = game_input_init();

    if (err != ERR_SUCCESS) {
        printf("INPUT INIT ERROR: %d\n", (int)err);
        return 1;
    }

    if (render_video_init() != 0) {
        render_shutdown();
        printf("VIDEO INIT ERROR\n");
        return 1;
    }

    /* Title screen: loads its own palette/tileset/tilemap, fades in and out.
     * Clear sprites first: after a warm restart the video board still holds
     * the previous run's HUD text. */
    render_hud_hide();

    if (splash_show(render_context())) {
        gfx_enable_screen(0);
    }

    if (render_assets_init() != 0) {
        render_shutdown();
        printf("VIDEO INIT ERROR\n");
        return 1;
    }

    zsfx_init();
    entropy_seed_random();
    scores_load();

    /* Events come from input.c. Mouse support can later map the pointer
     * position to cursor_x/cursor_y and synthesize the same actions. */
    while (1) {
        if (!save_load(&play.game))
            game_init(&play.game);

        render_hud_init(&play.game);
        render_invalidate();
        play_reset(&play);
        gfx_enable_screen(1);
        game_input_clear();

        pending = 0;

        while (1) {
            uint16_t mouse_buttons = 0;

            events = game_input_events();
            game_input_mouse(&mouse);

            if (mouse.moved) {
                /* Track the pointer even during animations so it stays in sync
                 * with the physical mouse; the cursor catches up when idle. */
                play_mouse_track(&play, mouse.dx, mouse.dy);
            }

            if (events & BUTTON_SELECT) {
                zsfx_stop_all();
                render_shutdown();
                return 0;
            }

            if (mouse.buttons & BUTTON_A) {
                if (render_hud_button_hit((uint16_t)play.mouse_x,
                                          (uint16_t)play.mouse_y))
                    mouse_buttons |= BUTTON_Y;   /* clickable NEW GAME */
                else if (render_board_hit((uint16_t)play.mouse_x,
                                          (uint16_t)play.mouse_y))
                    mouse_buttons |= BUTTON_A;
            }

            if (mouse.buttons & BUTTON_B)
                mouse_buttons |= BUTTON_B;

            if (play.state == PLAY_IDLE) {
                play_mouse_apply(&play);

                events |= mouse_buttons;

                if (pending) {
                    /* The newest press wins on each axis; the queued one only
                     * fills in axes and buttons that were not pressed again. */
                    if (!(events & (BUTTON_LEFT | BUTTON_RIGHT)))
                        events |= (uint16_t)(pending &
                                             (BUTTON_LEFT | BUTTON_RIGHT));
                    if (!(events & (BUTTON_UP | BUTTON_DOWN)))
                        events |= (uint16_t)(pending &
                                             (BUTTON_UP | BUTTON_DOWN));
                    if (!(events & (BUTTON_A | BUTTON_B | BUTTON_START)))
                        events |= (uint16_t)(pending &
                                             (BUTTON_A | BUTTON_B |
                                              BUTTON_START));
                    pending = 0;
                }

                play_input(&play, events);
            } else {
                /* Keep the newest press so a tap during a move, clear or spawn
                 * animation is applied as soon as the board is idle again.
                 * Older presses are dropped instead of piling up. */
                uint16_t press = (uint16_t)((events | mouse.buttons) &
                    (BUTTON_LEFT | BUTTON_RIGHT | BUTTON_UP | BUTTON_DOWN |
                     BUTTON_A | BUTTON_B | BUTTON_START));

                if (press)
                    pending = press;
            }

            play_tick(&play);
            zsfx_update();

            if (play.game.game_over) {
                zsfx_game_over();

                if (gameover_run(&play.game)) {
                    zsfx_stop_all();
                    break;
                }

                zsfx_stop_all();
                render_shutdown();
                return 0;
            }

            /* msleep keeps the loop usable in headless emulator sessions,
             * where no video vblank is generated. */
            msleep(16);
        }
    }
}
