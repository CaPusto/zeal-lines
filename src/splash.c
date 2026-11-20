#include <stdint.h>
#include <string.h>

#include <zos_errors.h>
#include <zos_vfs.h>

#include <zgdk.h>
#include <zvb_gfx.h>

#include "input.h"
#include "splash.h"

#define SPLASH_ZTP  "splash.ztp"
#define SPLASH_ZTS  "splash.zts"
#define SPLASH_ZTM  "splash.ztm"

#define SPLASH_CHUNK    512
#define SPLASH_PAL_LEN  16
#define SPLASH_HOLD_SEC 25
#define SPLASH_FRAMES   (SPLASH_HOLD_SEC * 60)

#define SPLASH_ROWS     30
#define SPLASH_COLS     40

static uint8_t splash_buf[SPLASH_CHUNK];
static uint16_t splash_pal_target[SPLASH_PAL_LEN];
static uint16_t splash_pal_current[SPLASH_PAL_LEN];


/*
 * Look for the asset next to the executable the same way the game loads its
 * tileset: emulator HostFS first, then the current directory and the usual
 * drives.
 */
static zos_dev_t open_splash(const char* name)
{
    static const char* roots[] = { "H:/", "", "T:/", "A:/" };
    char path[16];
    uint8_t i;

    for (i = 0; i < sizeof(roots) / sizeof(roots[0]); i++) {
        zos_dev_t dev;

        strcpy(path, roots[i]);
        strcat(path, name);

        dev = open(path, O_RDONLY);

        if (dev >= 0)
            return dev;
    }

    return -1;
}


static void splash_apply_brightness(gfx_context* ctx, uint8_t level)
{
    uint8_t i;

    for (i = 0; i < SPLASH_PAL_LEN; i++) {
        const uint16_t c = splash_pal_target[i];
        const uint16_t r = (uint16_t)((((c >> 11) & 0x1F) * level) >> 5);
        const uint16_t g = (uint16_t)((((c >> 5) & 0x3F) * level) >> 5);
        const uint16_t b = (uint16_t)(((c & 0x1F) * level) >> 5);

        splash_pal_current[i] = (uint16_t)((r << 11) | (g << 5) | b);
    }

    gfx_palette_load(ctx, splash_pal_current, sizeof(splash_pal_current), 0);
}


/* msleep() keeps the splash working in headless emulator sessions, where no
 * video vblank is generated. */
static void splash_wait_frame(void)
{
    msleep(16);
}


static uint8_t splash_load(gfx_context* ctx)
{
    zos_dev_t dev;
    uint16_t size;
    uint16_t off;
    uint8_t y;

    /* Palette: 16 RGB565 entries in palette slot 0. */
    dev = open_splash(SPLASH_ZTP);

    if (dev < 0)
        return 0;

    size = (uint16_t)sizeof(splash_pal_target);

    if (read(dev, splash_pal_target, &size) != ERR_SUCCESS ||
        size < sizeof(splash_pal_target)) {
        close(dev);
        return 0;
    }

    close(dev);
    splash_apply_brightness(ctx, 0);

    /* Tileset: load straight into the video board, one chunk at a time. */
    dev = open_splash(SPLASH_ZTS);

    if (dev < 0)
        return 0;

    off = 0;

    for (;;) {
        size = SPLASH_CHUNK;

        if (read(dev, splash_buf, &size) != ERR_SUCCESS || size == 0)
            break;

        if (gfx_tileset_load_none(ctx, splash_buf, size, off, 0, 0) !=
            GFX_SUCCESS) {
            close(dev);
            return 0;
        }

        off = (uint16_t)(off + size);
    }

    close(dev);

    /* Tilemap: 30 tile rows for layer 0, then 30 attribute rows for layer 1. */
    dev = open_splash(SPLASH_ZTM);

    if (dev < 0)
        return 0;

    for (y = 0; y < SPLASH_ROWS; y++) {
        size = SPLASH_COLS;

        if (read(dev, splash_buf, &size) != ERR_SUCCESS ||
            size < SPLASH_COLS) {
            close(dev);
            return 0;
        }

        gfx_tilemap_load(ctx, splash_buf, SPLASH_COLS, 0, 0, y);
    }

    for (y = 0; y < SPLASH_ROWS; y++) {
        size = SPLASH_COLS;

        if (read(dev, splash_buf, &size) != ERR_SUCCESS ||
            size < SPLASH_COLS) {
            close(dev);
            return 0;
        }

        gfx_tilemap_load(ctx, splash_buf, SPLASH_COLS, 1, 0, y);
    }

    close(dev);

    return 1;
}


uint8_t splash_show(gfx_context* ctx)
{
    uint16_t frames = 0;
    uint8_t level;

    if (!splash_load(ctx))
        return 0;

    /* Fade in from black. */
    gfx_enable_screen(1);

    for (level = 0; level <= 32; level++) {
        splash_wait_frame();
        splash_apply_brightness(ctx, level);
        frames++;
    }

    /* Wait for any key, mouse click, or continue after the hold time. */
    {
        game_mouse_t mouse;

        while (frames < SPLASH_FRAMES) {
            if (game_input_events() != 0)
                break;

            game_input_mouse(&mouse);
            if (mouse.buttons != 0)
                break;

            splash_wait_frame();
            frames++;
        }
    }

    /* Fade out to black. */
    for (level = 32; level != 0xff; level--) {
        splash_wait_frame();
        splash_apply_brightness(ctx, level);
    }

    return 1;
}
