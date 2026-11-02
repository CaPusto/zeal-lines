#include <stdint.h>
#include <stdio.h>

#include <zos_errors.h>
#include <zos_vfs.h>

#include <zgdk.h>
#include <zgdk/file.h>
#include <zvb_gfx.h>

#include "assets_generated.h"

#define TILE_BUFFER_SIZE 2048
#define GRID_SIZE 10

static gfx_context video;

/*
 * Возвращает номер группы палитры для варианта.
 *
 * 0       — обычная пустая клетка
 * 1       — выделенная пустая клетка
 * 2..8    — обычные шарики
 * 9..15   — выделенные шарики
 */
static uint8_t get_palette_group(uint8_t variant)
{
    uint8_t n;

    if (variant < 2) {
        return variant;
    }

    n = variant - 2;

    return 2 + (n / 14) + ((n % 14) >= 7 ? 7 : 0);
}

/* Загрузка графического файла с диска в видеопамять */
static int load_tileset(void)
{
    static uint8_t buffer[TILE_BUFFER_SIZE];

    zos_dev_t dev;
    zos_err_t err;

    uint16_t offset = 0;
    uint16_t total_size =
        ZEAL_ASSET_TILE_COUNT * ZEAL_ASSET_TILE_BYTES;

    dev = open("H:/zeal_tiles.bin", O_RDONLY);

    if (dev < 0) {
        printf("TILESET OPEN ERROR: %d\n", (int)dev);
        return 1;
    }

    while (offset < total_size) {
        uint16_t amount = total_size - offset;
        uint16_t received;

        if (amount > TILE_BUFFER_SIZE) {
            amount = TILE_BUFFER_SIZE;
        }

        received = amount;

        err = file_read_safe(dev, buffer, &received);

        if (err != ERR_SUCCESS) {
            printf("TILESET READ ERROR: %d\n", (int)err);
            close(dev);
            return 1;
        }

        if (received != amount ||
            received == 0 ||
            (received % ZEAL_ASSET_TILE_BYTES) != 0) {
            printf("TILESET SIZE ERROR\n");
            close(dev);
            return 1;
        }

        if (gfx_tileset_load_none(
                &video,
                buffer,
                received,
                offset,
                0,
                0) != GFX_SUCCESS) {
            printf("TILESET VRAM ERROR\n");
            close(dev);
            return 1;
        }

        offset += received;
    }

    close(dev);
    return 0;
}

/* Построение и вывод тестовой сетки */
static int draw_test_grid(void)
{
    static uint8_t tilemap[40];
    static uint8_t attributes[40];

    uint8_t screen_y;
    uint8_t x;

    for (screen_y = 0; screen_y < 30; screen_y++) {

        /* По умолчанию — обычная пустая клетка */
        for (x = 0; x < 40; x++) {
            tilemap[x] = 0;
            attributes[x] = 0;
        }

        /*
         * Сетка начинается с координаты:
         * X = 10 тайлов (160 пикселей)
         * Y = 5 тайлов (80 пикселей)
         */
        if (screen_y >= 5 && screen_y < 25) {

            uint8_t grid_y = (screen_y - 5) / 2;
            uint8_t local_y = (screen_y - 5) % 2;

            for (uint8_t grid_x = 0; grid_x < GRID_SIZE; grid_x++) {

                uint8_t variant = grid_y * GRID_SIZE + grid_x;
                uint8_t palette_group =
                    get_palette_group(variant);

                for (uint8_t local_x = 0; local_x < 2; local_x++) {

                    uint16_t tile_index =
                        variant * 4 + local_y * 2 + local_x;

                    uint8_t screen_x =
                        10 + grid_x * 2 + local_x;

                    tilemap[screen_x] = (uint8_t)tile_index;

                    attributes[screen_x] =
                        (palette_group << 4) |
                        ((tile_index >> 8) & 1);
                }
            }
        }

        if (gfx_tilemap_load(
                &video,
                tilemap,
                sizeof(tilemap),
                0,
                0,
                screen_y) != GFX_SUCCESS) {
            printf("TILEMAP ERROR\n");
            return 1;
        }

        if (gfx_tilemap_load(
                &video,
                attributes,
                sizeof(attributes),
                1,
                0,
                screen_y) != GFX_SUCCESS) {
            printf("ATTRIBUTE ERROR\n");
            return 1;
        }
    }

    return 0;
}

int main(void)
{
    zos_err_t err;

    /* Инициализация ввода */
    err = input_init(1);

    if (err != ERR_SUCCESS) {
        printf("INPUT INIT ERROR: %d\n", (int)err);
        return 1;
    }

    /* Выключаем экран на время настройки */
    gfx_enable_screen(0);

    /* Инициализация режима 640x480, 4 bpp */
    if (gfx_initialize(
            ZVB_CTRL_VID_MODE_GFX_640_4BIT,
            &video) != GFX_SUCCESS) {
        printf("VIDEO INIT ERROR\n");
        return 1;
    }

    /* Загружаем все 256 цветов палитры */
    if (gfx_palette_load(
            &video,
            (void*)zeal_palette,
            sizeof(zeal_palette),
            0) != GFX_SUCCESS) {
        printf("PALETTE ERROR\n");
        return 1;
    }

    /* Загружаем графические тайлы */
    if (load_tileset() != 0) {
        return 1;
    }

    /* Выводим тестовую сетку */
    if (draw_test_grid() != 0) {
        return 1;
    }

    /* Показываем изображение */
    gfx_enable_screen(1);

    /* Ожидание SELECT */
    while (1) {
        uint16_t buttons = input_read();

        if (buttons & BUTTON_SELECT) {
            break;
        }
    }

    return 0;
}