#pragma once

#include <stdint.h>

/* Number of entries kept in the high-score table. */
#define HIGH_SCORE_COUNT 5

/*
 * Load the table from zeal-scores.dat (H:, current directory, T:).
 * A missing or invalid file leaves an empty table and returns 0.
 */
uint8_t scores_load(void);

/*
 * Insert a score, keeping the table sorted from highest to lowest.
 * Returns the rank 0..HIGH_SCORE_COUNT-1 of the new entry, or 0xFF when the
 * score did not make the table. The file is written when something changes.
 */
uint8_t scores_insert(uint16_t score);

/* Score at the given rank; 0 for an empty slot. */
uint16_t scores_get(uint8_t rank);
