#pragma once

#include <stdint.h>

void zsfx_init(void);
void zsfx_update(void);
void zsfx_stop_all(void);

void zsfx_select(void);
void zsfx_move(void);
void zsfx_spawn(void);
void zsfx_clear(uint8_t count);
void zsfx_game_over(void);
void zsfx_new_game(void);
