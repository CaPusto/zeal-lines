
#include <stdint.h>
#include <stdio.h>

#include <zos_errors.h>
#include <zgdk.h>
#include <zvb_gfx.h>

static gfx_context video;

/* 16 цветов RGB565 */
static const uint16_t test_palette[16] = {
    0x0000, /*  0 Black       */
    0xFFFF, /*  1 White       */
    0xF800, /*  2 Red         */
    0x07E0, /*  3 Green       */
    0x001F, /*  4 Blue        */
    0xFFE0, /*  5 Yellow      */
    0xFC00, /*  6 Orange      */
    0x801F, /*  7 Purple      */
    0x07FF, /*  8 Cyan        */
    0xF81F, /*  9 Magenta     */
    0x8410, /* 10 Gray        */
    0x4208, /* 11 Dark gray   */
    0x87F0, /* 12 Light green */
    0xFC10, /* 13 Pink        */
    0x841F, /* 14 Light blue  */
    0xFFF0  /* 15 Pale yellow */
};

int main(void)
{
    /* Инициализация ввода */
    zos_err_t err = input_init(1);
    if (err != ERR_SUCCESS) {
        printf("INPUT INIT ERROR: %d\n", err);
        return 1;
    }

    /* Выключаем экран на время настройки */
    gfx_enable_screen(0);

    /* Режим 640x480, 4 bpp */
    if (gfx_initialize(
            ZVB_CTRL_VID_MODE_GFX_640_4BIT,
            &video) != GFX_SUCCESS) {
        printf("VIDEO INIT ERROR\n");
        return 1;
    }

    /* Загружаем палитру */
    if (gfx_palette_load(
            &video,
            (void*)test_palette,
            sizeof(test_palette),
            0) != GFX_SUCCESS) {
        printf("PALETTE ERROR\n");
        return 1;
    }

    /* Создаём 16 одноцветных тайлов.
       В 4 bpp один байт содержит два пикселя,
       поэтому используем i * 0x11. */
    for (uint8_t i = 0; i < 16; i++) {
        if (gfx_tileset_add_color_tile(
                &video, i, (uint8_t)(i * 0x11)) != GFX_SUCCESS) {
            printf("TILE ERROR: %u\n", i);
            return 1;
        }
    }

    /* 40 тайлов по горизонтали = 640 пикселей */
    static uint8_t tilemap[40];
    static uint8_t attributes[40];

    /* Заполняем карту вертикальными цветными полосами */
    for (uint8_t x = 0; x < 40; x++) {
        tilemap[x] = x % 16;
        attributes[x] = 0;
    }

    /* 30 рядов по вертикали = 480 пикселей */
    for (uint8_t y = 0; y < 30; y++) {
        if (gfx_tilemap_load(
                &video, tilemap, sizeof(tilemap), 0, 0, y) != GFX_SUCCESS) {
            printf("TILEMAP ERROR\n");
            return 1;
        }

        /* В режиме 4 bpp layer1 — это атрибуты тайлов */
        if (gfx_tilemap_load(
                &video, attributes, sizeof(attributes), 1, 0, y) != GFX_SUCCESS) {
            printf("ATTRIBUTE ERROR\n");
            return 1;
        }
    }

    /* Включаем изображение */
    gfx_enable_screen(1);

    while (1) {
        uint16_t buttons = input_read();

        if (buttons & BUTTON_SELECT) {
            break;
        }
    }

    return 0;
}
