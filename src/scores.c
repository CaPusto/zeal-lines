#include <stdint.h>

#include <zos_errors.h>
#include <zos_vfs.h>

#include "scores.h"

#define SCORES_MAGIC_0 'Z'
#define SCORES_MAGIC_1 'L'
#define SCORES_MAGIC_2 'S'
#define SCORES_MAGIC_3 '1'

#define SCORES_SIZE  (4 + HIGH_SCORE_COUNT * 2 + 1)

static const char* score_paths[] = {
    "H:/zeal-scores.dat",
    "zeal-scores.dat",
    "T:/zeal-scores.dat"
};

#define SCORE_PATH_COUNT (sizeof(score_paths) / sizeof(score_paths[0]))

static uint16_t high_scores[HIGH_SCORE_COUNT];


static uint8_t checksum(const uint8_t* buffer, uint16_t size)
{
    uint8_t result = 0x5A;
    uint16_t i;

    for (i = 0; i < size; i++)
        result = (uint8_t)(result ^ buffer[i]);

    return result;
}


static void scores_clear_memory(void)
{
    uint8_t i;

    for (i = 0; i < HIGH_SCORE_COUNT; i++)
        high_scores[i] = 0;
}


uint8_t scores_load(void)
{
    uint8_t buffer[SCORES_SIZE];
    zos_dev_t dev = -1;
    uint16_t size;
    uint8_t i;

    scores_clear_memory();

    for (i = 0; i < SCORE_PATH_COUNT; i++) {
        dev = open(score_paths[i], O_RDONLY);
        if (dev >= 0)
            break;
    }

    if (dev < 0)
        return 0;

    size = SCORES_SIZE;

    if (read(dev, buffer, &size) != ERR_SUCCESS || size != SCORES_SIZE) {
        close(dev);
        return 0;
    }

    close(dev);

    if (buffer[0] != SCORES_MAGIC_0 ||
        buffer[1] != SCORES_MAGIC_1 ||
        buffer[2] != SCORES_MAGIC_2 ||
        buffer[3] != SCORES_MAGIC_3)
        return 0;

    if (checksum(buffer, SCORES_SIZE - 1) != buffer[SCORES_SIZE - 1])
        return 0;

    for (i = 0; i < HIGH_SCORE_COUNT; i++) {
        high_scores[i] = (uint16_t)(buffer[4 + i * 2] |
                                    (buffer[5 + i * 2] << 8));
    }

    return 1;
}


static void scores_save(void)
{
    uint8_t buffer[SCORES_SIZE];
    zos_dev_t dev = -1;
    uint16_t size;
    uint8_t i;
    uint8_t pos;

    buffer[0] = SCORES_MAGIC_0;
    buffer[1] = SCORES_MAGIC_1;
    buffer[2] = SCORES_MAGIC_2;
    buffer[3] = SCORES_MAGIC_3;

    pos = 4;

    for (i = 0; i < HIGH_SCORE_COUNT; i++) {
        buffer[pos++] = (uint8_t)(high_scores[i] & 0xFF);
        buffer[pos++] = (uint8_t)(high_scores[i] >> 8);
    }

    buffer[pos] = checksum(buffer, pos);

    for (i = 0; i < SCORE_PATH_COUNT; i++) {
        dev = open(score_paths[i], O_WRONLY | O_CREAT | O_TRUNC);
        if (dev >= 0)
            break;
    }

    if (dev < 0)
        return;

    size = SCORES_SIZE;
    write(dev, buffer, &size);
    close(dev);
}


uint8_t scores_insert(uint16_t score)
{
    uint8_t position;
    uint8_t i;

    if (score == 0)
        return 0xFF;

    position = HIGH_SCORE_COUNT;

    for (i = 0; i < HIGH_SCORE_COUNT; i++) {
        if (score > high_scores[i]) {
            position = i;
            break;
        }
    }

    if (position == HIGH_SCORE_COUNT)
        return 0xFF;

    for (i = HIGH_SCORE_COUNT - 1; i > position; i--)
        high_scores[i] = high_scores[i - 1];

    high_scores[position] = score;
    scores_save();

    return position;
}


uint16_t scores_get(uint8_t rank)
{
    if (rank >= HIGH_SCORE_COUNT)
        return 0;

    return high_scores[rank];
}
