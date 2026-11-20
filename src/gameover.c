#include <stdint.h>

#include <zgdk.h>

#include "assets_generated.h"
#include "gameover.h"
#include "input.h"
#include "render.h"
#include "scores.h"
#include "sfx.h"

#define POPUP_FRAMES  180   /* auto-advance after ~3 seconds */

#define SCREEN_W      640
#define CHAR_W        8
#define DIGIT_W       16


static uint16_t poll_events(void)
{
    game_mouse_t mouse;
    uint16_t events;

    events = game_input_events();
    game_input_mouse(&mouse);

    return (uint16_t)(events | mouse.buttons);
}


static uint8_t text_length(const char* text)
{
    uint8_t length = 0;

    while (text[length] != 0)
        length++;

    return length;
}


static int16_t center_x(const char* text, uint8_t spacing)
{
    return (int16_t)((SCREEN_W -
                      (int16_t)text_length(text) * spacing) / 2);
}


static uint8_t number_digits(uint16_t value)
{
    uint8_t count = 1;

    while (value >= 10) {
        value = (uint16_t)(value / 10);
        count++;
    }

    return count;
}


/* Score digits are 16x16 tiles, so they need 16 px of sprite spacing. */
static void draw_number(int16_t x, int16_t y, uint16_t value)
{
    char digits[5];
    uint8_t count = 0;
    uint8_t i;

    if (value == 0) {
        digits[count++] = '0';
    } else {
        while (value != 0 && count < 5) {
            digits[count++] = (char)('0' + (value % 10));
            value = (uint16_t)(value / 10);
        }
    }

    for (i = 0; i < count; i++) {
        char one[2];

        one[0] = digits[count - 1 - i];
        one[1] = 0;
        render_sprites_text((int16_t)(x + i * DIGIT_W), y, one);
    }
}


/* A colorful divider made from the NEXT preview balls. */
static void draw_ball_row(int16_t y, uint8_t count)
{
    int16_t width = (int16_t)(count * 16 + (count - 1) * 8);
    int16_t x = (int16_t)((SCREEN_W - width) / 2);
    uint8_t i;

    for (i = 0; i < count; i++) {
        render_sprites_tile((int16_t)(x + i * 24), y,
                            (uint16_t)(TILE_PREVIEW + i),
                            (uint8_t)(PALETTE_GROUP_BALL + i));
    }
}


static void draw_popup(uint16_t score, uint8_t rank)
{
    uint8_t digits;

    render_sprites_begin();

    /* Beveled modal: mid-grey rim, dark inner rim, black content area. */
    render_box(7, 7, 26, 16, TILE_BUTTON);
    render_box(8, 8, 24, 14, TILE_PANEL);
    render_box(9, 9, 22, 12, TILE_BLACK);

    render_sprites_text_spaced(center_x("GAME OVER", 14), 154,
                               "GAME OVER", 14);
    draw_ball_row(184, 7);

    render_sprites_text(center_x("SCORE", 8), 216, "SCORE");

    digits = number_digits(score);
    draw_number((int16_t)((SCREEN_W - digits * DIGIT_W) / 2), 232, score);

    if (rank == 0) {
        render_sprites_text_color(center_x("NEW HIGH SCORE", 8), 272,
                                  "NEW HIGH SCORE", 8, PALETTE_GROUP_BALL);
    } else if (rank < HIGH_SCORE_COUNT) {
        render_sprites_text_color(center_x("TOP FIVE", 8), 272,
                                  "TOP FIVE", 8, PALETTE_GROUP_BALL);
    }

    render_sprites_text_spaced(center_x("PRESS ANY KEY", 10), 304,
                               "PRESS ANY KEY", 10);
}


static void draw_table(uint8_t rank)
{
    uint8_t i;

    render_sprites_begin();
    render_screen_fill(TILE_BLACK);

    /* Framed screen: a grey rim around the black page. */
    render_box(2, 1, 36, 28, TILE_BUTTON);
    render_box(3, 2, 34, 26, TILE_BLACK);

    draw_ball_row(52, 7);

    render_sprites_text_spaced(center_x("HIGH SCORES", 12), 80,
                               "HIGH SCORES", 12);
    render_box(11, 7, 18, 1, TILE_PANEL);

    for (i = 0; i < HIGH_SCORE_COUNT; i++) {
        uint8_t tile_y = (uint8_t)(9 + i * 2);
        int16_t y = (int16_t)(tile_y * 16);
        uint16_t value = scores_get(i);
        char label[2];

        /* Rank chip and its digit. */
        render_box(15, tile_y, 1, 1, TILE_PANEL);
        label[0] = (char)('1' + i);
        label[1] = 0;
        render_sprites_text(240, y, label);

        draw_number((int16_t)(368 - number_digits(value) * DIGIT_W), y, value);

        if (i == rank) {
            render_sprites_tile(196, y, (uint16_t)TILE_PREVIEW,
                                PALETTE_GROUP_BALL);
            render_sprites_text_color(384, (int16_t)(y + 4), "NEW", 8,
                                      PALETTE_GROUP_BALL);
        }
    }

    render_box(11, 20, 18, 1, TILE_PANEL);

    render_sprites_text(center_x("A - NEW GAME", 8), 344, "A - NEW GAME");
    render_sprites_text(center_x("SELECT - EXIT", 8), 368, "SELECT - EXIT");
}


uint8_t gameover_run(game_t* game)
{
    uint8_t rank;
    uint16_t frames;

    rank = scores_insert(game->score);

    draw_popup(game->score, rank);

    frames = POPUP_FRAMES;

    while (frames != 0) {
        if (poll_events() != 0)
            break;

        zsfx_update();
        msleep(16);
        frames--;
    }

    draw_table(rank);

    for (;;) {
        uint16_t events = poll_events();

        if (events & BUTTON_SELECT)
            return 0;

        if (events & (BUTTON_A | BUTTON_B | BUTTON_START | BUTTON_Y))
            return 1;

        zsfx_update();
        msleep(16);
    }
}
