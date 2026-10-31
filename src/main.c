
#include <stdint.h>
#include <stdio.h>

#include <zos_errors.h>
#include <zgdk.h>
#include <zvb_gfx.h>

static gfx_context video;

static const uint16_t test_palette[16] = {
    RGB888_TO_RGB565(0,   0,   0),
    RGB888_TO_RGB565(255, 255, 255),
    RGB888_TO_RGB565(255, 0,   0),
    RGB888_TO_RGB565(0,   255, 0),
    RGB888_TO_RGB565(0,   0,   255),
    RGB888_TO_RGB565(255, 255, 0),
    RGB888_TO_RGB565(255, 128, 0),
    RGB888_TO_RGB565(128, 0,   255),
    RGB888_TO_RGB565(0,   255, 255),
    RGB888_TO_RGB565(255, 0,   255),
    RGB888_TO_RGB565(128, 128, 128),
    RGB888_TO_RGB565(64,  64,  64),
    RGB888_TO_RGB565(128, 255, 128),
    RGB888_TO_RGB565(255, 128, 128),
    RGB888_TO_RGB565(128, 128, 255),
    RGB888_TO_RGB565(255, 255, 128)
};

int main(void)
{
    zos_err_t err;

    /* Initialize keyboard and SNES controller */
    err = input_init(1);
    if (err != ERR_SUCCESS) {
        printf("INPUT INIT ERROR: %d\n", err);
        return 1;
    }

    /* Disable video output during initialization */
    gfx_enable_screen(0);

    /* Initialize 640x480, 4-bit graphics */
    if (gfx_initialize(
            ZVB_CTRL_VID_MODE_GFX_640_4BIT,
            &video) != GFX_SUCCESS) {
        printf("VIDEO INIT ERROR\n");
        return 1;
    }

    /* Load first 16 palette entries */
    if (gfx_palette_load(
            &video,
            (void*)test_palette,
            sizeof(test_palette),
            0) != GFX_SUCCESS) {
        printf("PALETTE ERROR\n");
        return 1;
    }

    gfx_enable_screen(1);

    printf("ZEAL COLOR LINES TEST\n");
    printf("VIDEO: 640x480 4-BIT\n");
    printf("INPUT: KEYBOARD + SNES\n");
    printf("PRESS SELECT TO EXIT\n");

    while (1) {
        uint16_t buttons = input_read();

        if (buttons & BUTTON_SELECT) {
            break;
        }

        if (buttons & BUTTON_UP) {
            /* Test input */
        }

        if (buttons & BUTTON_DOWN) {
            /* Test input */
        }

        if (buttons & BUTTON_LEFT) {
            /* Test input */
        }

        if (buttons & BUTTON_RIGHT) {
            /* Test input */
        }

        if (buttons & BUTTON_A) {
            /* Test input */
        }

        if (buttons & BUTTON_B) {
            /* Test input */
        }
    }

    return 0;
}
