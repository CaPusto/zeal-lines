#pragma once

#include <stdint.h>

/* Host-side stubs for the ZGDK random helpers used by game.c. */
char rand8(void);
void rand8_seed(uint16_t seed);
char rand8_quick(void);
