#include <stdint.h>
#include <string.h>

#include <zos_errors.h>
#include <zos_vfs.h>

#include "save.h"

#define SAVE_SIZE  91
#define SAVE_MAGIC_0 'Z'
#define SAVE_MAGIC_1 'L'
#define SAVE_MAGIC_2 'N'
#define SAVE_MAGIC_3 '1'

static const char* save_paths[] = {
    "H:/zeal-lines.sav",
    "zeal-lines.sav",
    "T:/zeal-lines.sav"
};

#define SAVE_PATH_COUNT (sizeof(save_paths) / sizeof(save_paths[0]))


static zos_dev_t open_existing(void)
{
    uint8_t i;

    for (i = 0; i < SAVE_PATH_COUNT; i++) {
        zos_dev_t dev = open(save_paths[i], O_RDONLY);
        if (dev >= 0)
            return dev;
    }

    return -1;
}


static zos_dev_t open_for_write(void)
{
    uint8_t i;

    for (i = 0; i < SAVE_PATH_COUNT; i++) {
        zos_dev_t dev = open(save_paths[i],
                             O_WRONLY | O_CREAT | O_TRUNC);
        if (dev >= 0)
            return dev;
    }

    return -1;
}


static uint8_t checksum(const uint8_t* buffer, uint16_t size)
{
    uint8_t result = 0x5A;
    uint16_t i;

    for (i = 0; i < size; i++)
        result = (uint8_t)(result ^ buffer[i]);

    return result;
}


uint8_t save_load(game_t* game)
{
    uint8_t buffer[SAVE_SIZE];
    zos_dev_t dev;
    uint16_t size;
    uint8_t x;
    uint8_t y;
    uint8_t i;

    dev = open_existing();

    if (dev < 0)
        return 0;

    size = SAVE_SIZE;
    if (read(dev, buffer, &size) != ERR_SUCCESS || size != SAVE_SIZE) {
        close(dev);
        return 0;
    }

    close(dev);

    if (buffer[0] != SAVE_MAGIC_0 ||
        buffer[1] != SAVE_MAGIC_1 ||
        buffer[2] != SAVE_MAGIC_2 ||
        buffer[3] != SAVE_MAGIC_3)
        return 0;

    if (checksum(buffer, SAVE_SIZE - 1) != buffer[SAVE_SIZE - 1])
        return 0;

    for (y = 0; y < GRID_SIZE; y++) {
        for (x = 0; x < GRID_SIZE; x++) {
            uint8_t value = buffer[4 + y * GRID_SIZE + x];
            if (value > COLOR_COUNT)
                return 0;
            game->cells[y][x] = value;
        }
    }

    for (i = 0; i < PREVIEW_COUNT; i++) {
        uint8_t value = buffer[4 + GRID_SIZE * GRID_SIZE + i];
        if (value < 1 || value > COLOR_COUNT)
            return 0;
        game->preview[i] = value;
    }

    game->score = (uint16_t)(buffer[4 + GRID_SIZE * GRID_SIZE +
                                       PREVIEW_COUNT] |
                             (buffer[4 + GRID_SIZE * GRID_SIZE +
                                     PREVIEW_COUNT + 1] << 8));
    game->game_over = 0;

    if (game_board_full(game))
        return 0;

    return 1;
}


void save_store(const game_t* game)
{
    uint8_t buffer[SAVE_SIZE];
    zos_dev_t dev;
    uint16_t size;
    uint8_t x;
    uint8_t y;
    uint8_t i;
    uint8_t pos;

    buffer[0] = SAVE_MAGIC_0;
    buffer[1] = SAVE_MAGIC_1;
    buffer[2] = SAVE_MAGIC_2;
    buffer[3] = SAVE_MAGIC_3;

    pos = 4;
    for (y = 0; y < GRID_SIZE; y++) {
        for (x = 0; x < GRID_SIZE; x++)
            buffer[pos++] = game->cells[y][x];
    }

    for (i = 0; i < PREVIEW_COUNT; i++)
        buffer[pos++] = game->preview[i];

    buffer[pos++] = (uint8_t)(game->score & 0xFF);
    buffer[pos++] = (uint8_t)(game->score >> 8);
    buffer[pos] = checksum(buffer, pos);

    dev = open_for_write();

    if (dev < 0)
        return;

    size = SAVE_SIZE;
    write(dev, buffer, &size);
    close(dev);
}


void save_clear(void)
{
    uint8_t i;

    for (i = 0; i < SAVE_PATH_COUNT; i++)
        rm(save_paths[i]);
}
