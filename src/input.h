#pragma once

#include <stdint.h>
#include <zos_errors.h>

/*
 * Event-based input.
 *
 * Unlike input_read() from ZGDK, which only exposes the current keyboard
 * state, this module returns new key presses even when the press and release
 * arrive between two polls. This matters when a frame takes long enough to
 * swallow a quick tap.
 */
zos_err_t game_input_init(void);

/* Newly pressed buttons since the previous call. */
uint16_t game_input_events(void);

/* Drain pending events and reset held-key state. */
void game_input_clear(void);

/* SNES mouse, connected to either controller port. Polling it never disables
 * the keyboard or the gamepad. */
typedef struct {
    uint8_t present;   /* a mouse was detected on a controller port */
    uint8_t moved;     /* dx/dy changed since the previous call */
    int16_t dx;        /* horizontal movement, screen pixels */
    int16_t dy;        /* vertical movement, screen pixels */
    uint16_t buttons;  /* new presses: BUTTON_A (left), BUTTON_B (right) */
} game_mouse_t;

/* Fill the mouse state for this frame. */
void game_input_mouse(game_mouse_t* mouse);
