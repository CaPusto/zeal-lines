#include <stdint.h>
#include <stdio.h>

#include <zos_errors.h>
#include <zos_vfs.h>

#include <zgdk.h>
#include <zgdk/file.h>
#include <zvb_gfx.h>

#include "assets_generated.h"

/* Размер буфера чтения тайлов */
#define TILE_BUFFER_SIZE 2048

/* Размер игрового поля */
#define GRID_SIZE 9

/* Размер клетки в тайлах: 3x3 тайла по 16x16 */
#define CELL_TILES 3

/* Положение поля на экране */
#define GRID_TILE_X 0
#define GRID_TILE_Y 1

/* Количество тайлов в одной клетке */
#define CELL_TILE_COUNT (CELL_TILES * CELL_TILES)

/* Варианты клетки */
#define CELL_EMPTY 0
#define CELL_SELECTED 1

/*
 * Номера тайлов:
 *
 * 0..8     - пустая клетка
 * 9..17    - выделенная пустая клетка
 * 18..143  - цветные клетки
 *
 * 144      - фоновый тайл
 */
#define BACKGROUND_TILE 144

static gfx_context video;


/*
 * Возвращает номер группы палитры для варианта клетки.
 *
 * Группы:
 * 0       - обычный фон
 * 1       - выделенный фон
 * 2..8    - обычные цветные клетки
 * 9..15   - выделенные цветные клетки
 */
static uint8_t get_palette_group(uint8_t variant)
{
    uint8_t n;

    if (variant < 2)
        return variant;

    n = variant - 2;

    return 2 + (n / 2) + ((n % 2) ? 7 : 0);
}


/*
 * Загрузка тайлов из zeal_tiles.bin.
 *
 * В файле находятся отдельные страницы анимации.
 * Каждая страница содержит 144 тайла.
 *
 * В видеопамять загружается только первая страница.
 * Последующие кадры можно будет загружать в те же
 * позиции видеопамяти, не занимая дополнительные
 * номера тайлов.
 */
static int load_tileset(void)
{
    static uint8_t buffer[TILE_BUFFER_SIZE];

    zos_dev_t dev;
    zos_err_t err;

    uint16_t offset = 0;

    /*
     * Размер одной страницы:
     * 144 тайла * 128 байт = 18432 байта.
     */
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

        if (amount > TILE_BUFFER_SIZE)
            amount = TILE_BUFFER_SIZE;

        received = amount;

        err = file_read_safe(dev, buffer, &received);

        if (err != ERR_SUCCESS ||
            received != amount ||
            received == 0 ||
            (received % ZEAL_ASSET_TILE_BYTES) != 0) {

            printf("TILESET READ ERROR\n");
            close(dev);
            return 1;
        }

        if (gfx_tileset_load_none(
                &video,
                buffer,
                received,
                offset,
                0,
                0
            ) != GFX_SUCCESS) {

            printf("TILESET VRAM ERROR\n");
            close(dev);
            return 1;
        }

        offset += received;
    }

    close(dev);

    /*
     * Создаём однотонный тайл для фона.
     * Он занимает номер 144, сразу после тайлов поля.
     */
    if (gfx_tileset_add_color_tile(
            &video,
            BACKGROUND_TILE,
            0
        ) != GFX_SUCCESS) {

        printf("BACKGROUND TILE ERROR\n");
        return 1;
    }

    return 0;
}


/*
 * Отрисовка игрового поля и курсора.
 *
 * Одна клетка занимает 3x3 тайла.
 * Поле занимает 27x27 тайлов.
 *
 * В режиме 640x480:
 * 40 тайлов по горизонтали,
 * 30 тайлов по вертикали.
 */
static int draw_board(uint8_t cursor_x, uint8_t cursor_y)
{
    static uint8_t tilemap[40];
    static uint8_t attributes[40];

    uint8_t screen_y;
    uint8_t x;

    for (screen_y = 0; screen_y < 30; screen_y++) {

        /*
         * Сначала заполняем всю строку фоновым тайлом.
         */
        for (x = 0; x < 40; x++) {

            tilemap[x] = BACKGROUND_TILE;
            attributes[x] = 0;
        }

        /*
         * Проверяем, попадает ли строка в игровое поле.
         */
        if (screen_y >= GRID_TILE_Y &&
            screen_y < GRID_TILE_Y + GRID_SIZE * CELL_TILES) {

            uint8_t cell_y =
                (screen_y - GRID_TILE_Y) / CELL_TILES;

            uint8_t local_y =
                (screen_y - GRID_TILE_Y) % CELL_TILES;

            uint8_t cell_x;

            /*
             * Обрабатываем все клетки текущей строки.
             */
            for (cell_x = 0; cell_x < GRID_SIZE; cell_x++) {

                uint8_t variant = CELL_EMPTY;

                /*
                 * Выделяем клетку под курсором.
                 */
                if (cell_x == cursor_x && cell_y == cursor_y)
                    variant = CELL_SELECTED;

                uint8_t palette_group =
                    get_palette_group(variant);

                uint8_t local_x;

                /*
                 * Одна клетка состоит из трёх тайлов
                 * по горизонтали и трёх по вертикали.
                 */
                for (local_x = 0; local_x < CELL_TILES; local_x++) {

                    uint16_t tile_index =
                        variant * CELL_TILE_COUNT +
                        local_y * CELL_TILES +
                        local_x;

                    uint8_t screen_x =
                        GRID_TILE_X +
                        cell_x * CELL_TILES +
                        local_x;

                    tilemap[screen_x] =
                        (uint8_t)tile_index;

                    /*
                     * Старший бит номера тайла
                     * и номер группы палитры.
                     */
                    attributes[screen_x] =
                        (palette_group << 4) |
                        ((tile_index >> 8) & 1);
                }
            }
        }

        /*
         * Загружаем строку карты тайлов.
         */
        if (gfx_tilemap_load(
                &video,
                tilemap,
                sizeof(tilemap),
                0,
                0,
                screen_y
            ) != GFX_SUCCESS) {

            printf("TILEMAP ERROR\n");
            return 1;
        }

        /*
         * Загружаем атрибуты строки.
         */
        if (gfx_tilemap_load(
                &video,
                attributes,
                sizeof(attributes),
                1,
                0,
                screen_y
            ) != GFX_SUCCESS) {

            printf("ATTRIBUTE ERROR\n");
            return 1;
        }
    }

    return 0;
}


/*
 * Главная функция
 */
int main(void)
{
    zos_err_t err;

    /* Начальная позиция курсора */
    uint8_t cursor_x = 4;
    uint8_t cursor_y = 4;

    /* Предыдущее состояние кнопок */
    uint16_t previous_buttons = 0;

    /*
     * Инициализация ввода.
     */
    err = input_init(1);

    if (err != ERR_SUCCESS) {
        printf("INPUT INIT ERROR: %d\n", (int)err);
        return 1;
    }

    /*
     * Выключаем экран на время настройки.
     */
    gfx_enable_screen(0);

    /*
     * Инициализация видеорежима:
     * 640x480, 4 bpp.
     */
    if (gfx_initialize(
            ZVB_CTRL_VID_MODE_GFX_640_4BIT,
            &video
        ) != GFX_SUCCESS) {

        printf("VIDEO INIT ERROR\n");
        return 1;
    }

    /*
     * Загрузка палитры.
     */
    if (gfx_palette_load(
            &video,
            (void*)zeal_palette,
            sizeof(zeal_palette),
            0
        ) != GFX_SUCCESS) {

        printf("PALETTE ERROR\n");
        return 1;
    }

    /*
     * Загрузка тайлов.
     */
    if (load_tileset() != 0)
        return 1;

    /*
     * Первоначальная отрисовка поля.
     */
    if (draw_board(cursor_x, cursor_y) != 0)
        return 1;

    input_flush();

    /*
     * Включаем экран.
     */
    gfx_enable_screen(1);

    /*
     * Главный цикл управления.
     */
    while (1) {

        uint16_t buttons = input_read();

        /*
         * Определяем новое нажатие,
         * а не удержание кнопки.
         */
        uint16_t pressed =
            buttons & ~previous_buttons;

        previous_buttons = buttons;

        /*
         * SELECT — выход.
         */
        if (pressed & BUTTON_SELECT)
            break;

        uint8_t changed = 0;

        /*
         * Перемещение курсора.
         */
        if (pressed & BUTTON_LEFT) {

            if (cursor_x > 0) {
                cursor_x--;
                changed = 1;
            }
        }
        else if (pressed & BUTTON_RIGHT) {

            if (cursor_x < GRID_SIZE - 1) {
                cursor_x++;
                changed = 1;
            }
        }
        else if (pressed & BUTTON_UP) {

            if (cursor_y > 0) {
                cursor_y--;
                changed = 1;
            }
        }
        else if (pressed & BUTTON_DOWN) {

            if (cursor_y < GRID_SIZE - 1) {
                cursor_y++;
                changed = 1;
            }
        }

        /*
         * Перерисовываем поле только при перемещении.
         */
        if (changed) {

            if (draw_board(cursor_x, cursor_y) != 0)
                return 1;
        }
    }

    return 0;
}