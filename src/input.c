#include <stdint.h>
#include <stddef.h>

#include <zos_errors.h>
#include <zos_vfs.h>
#include <zos_keyboard.h>
#include <zos_mouse.h>

#include <zgdk/input.h>

#include "input.h"

#define PAD_BUTTONS 0x0FFF
#define AXIS_X (BUTTON_LEFT | BUTTON_RIGHT)
#define AXIS_Y (BUTTON_UP | BUTTON_DOWN)

typedef struct {
    uint8_t code;
    uint16_t button;
} key_map_t;

static const key_map_t keys[] = {
    { KB_UP_ARROW,      BUTTON_UP },
    { KB_DOWN_ARROW,    BUTTON_DOWN },
    { KB_LEFT_ARROW,    BUTTON_LEFT },
    { KB_RIGHT_ARROW,   BUTTON_RIGHT },
    { KB_KEY_ENTER,     BUTTON_START },
    { '\r',             BUTTON_START },
    { KB_RIGHT_SHIFT,   BUTTON_SELECT },
    { KB_KEY_QUOTE,     BUTTON_SELECT },
    { KB_KEY_Z,         BUTTON_B },
    { KB_KEY_SPACE,     BUTTON_B },
    { KB_KEY_X,         BUTTON_A },
    { KB_KEY_A,         BUTTON_Y },
    { KB_KEY_S,         BUTTON_X },
    { KB_KEY_Q,         BUTTON_L },
    { KB_KEY_W,         BUTTON_R },
};

#define KEY_COUNT (sizeof(keys) / sizeof(keys[0]))

static uint8_t key_down[KEY_COUNT];
static uint8_t release_prefix;
static uint8_t input_buffer[32 + 31];
static uint8_t* input_ptr;

/*
 * Events collected since the previous poll.
 *
 * A held key generates typematic repeats: the keyboard controller (or the
 * emulator forwarding X11 auto-repeat) sends press/release pairs much faster
 * than one per frame. Returning every repeat as its own event made the game
 * apply a backlog of stale directions after the key was already released,
 * which looked like the cursor refusing to move or jumping back.
 *
 * Repeats are therefore folded per axis: only the most recent horizontal and
 * vertical direction of a frame survives.
 */
static uint16_t press_bits;
static uint16_t pad_last;
static uint16_t pad_suppressed;

/* A SNES mouse can sit on either controller port. 0xFF means "not found". */
#define MOUSE_PORT_NONE 0xFF
#define MOUSE_SCAN_FRAMES 120
#define MOUSE_LOST_FRAMES 30

static uint8_t mouse_port = MOUSE_PORT_NONE;
static uint8_t mouse_scan_timer;
static uint8_t mouse_lost;
static uint16_t mouse_last_buttons;

/* Fallback for mice handled by the OS driver (PS/2 on the extension board). */
static zos_dev_t mouse_dev = -1;

static void mouse_scan(void);
static void mouse_os_poll(game_mouse_t* mouse);


static void add_event(uint16_t button)
{
    if (button & AXIS_X)
        press_bits &= (uint16_t)~AXIS_X;
    else if (button & AXIS_Y)
        press_bits &= (uint16_t)~AXIS_Y;

    press_bits |= button;
}


static void parse_byte(uint8_t code)
{
    uint8_t i;

    if (code == KB_RELEASED) {
        release_prefix = 1;
        return;
    }

    for (i = 0; i < KEY_COUNT; i++) {
        if (keys[i].code != code)
            continue;

        if (release_prefix) {
            key_down[i] = 0;
        } else {
            /* Directions are reported on every repeat so holding a key
             * keeps moving. Buttons fire once per physical press. */
            if (!key_down[i] || (keys[i].button & (AXIS_X | AXIS_Y)))
                add_event(keys[i].button);

            key_down[i] = 1;
        }

        break;
    }

    release_prefix = 0;
}


static void drain_input(void)
{
    for (;;) {
        uint16_t size = 32;
        uint16_t i;
        zos_err_t err = read(DEV_STDIN, input_ptr, &size);

        if (err != ERR_SUCCESS || size == 0)
            break;

        for (i = 0; i < size; i++)
            parse_byte(input_ptr[i]);
    }
}


zos_err_t game_input_init(void)
{
    zos_err_t err;
    uint8_t i;

    err = input_init(1);

    if (err != ERR_SUCCESS)
        return err;

    for (i = 0; i < KEY_COUNT; i++)
        key_down[i] = 0;

    input_ptr = (uint8_t*)(((uintptr_t)(input_buffer + 31)) & ~(uintptr_t)31);

    release_prefix = 0;
    press_bits = 0;
    pad_last = 0;
    pad_suppressed = PAD_BUTTONS;

    /* A mouse on a port makes that port unusable for a gamepad, so find it
     * once at startup; game_input_mouse() rescans periodically for hot-plug. */
    mouse_port = MOUSE_PORT_NONE;
    mouse_scan_timer = 0;
    mouse_lost = 0;
    mouse_last_buttons = 0;
    mouse_scan();

    /* A PS/2 mouse is only reachable through the OS driver; it is used when
     * no SNES mouse answers on the controller ports. */
    mouse_dev = -1;
    if (mouse_port == MOUSE_PORT_NONE)
        mouse_dev = open(MOUSE_DEVICE_NAME, O_RDONLY);

    return ERR_SUCCESS;
}


/* Read every gamepad that is not sharing a port with the mouse. */
static uint16_t read_pads(uint8_t* any_valid)
{
    uint16_t pads = 0;
    uint8_t port;

    *any_valid = 0;

    for (port = 0; port < 2; port++) {
        uint16_t report;

        if (mouse_port == port)
            continue;

        report = controller_read_pad(port);

        if (controller_pad_valid(report)) {
            pads |= report;
            *any_valid = 1;
        }
    }

    return pads;
}


static uint16_t sample_pads(uint8_t report_edges)
{
    uint16_t pad;
    uint8_t valid;

    pad = read_pads(&valid);

    if (!valid) {
        pad_last = 0;
        pad_suppressed = PAD_BUTTONS;
        return 0;
    }

    if (report_edges) {
        uint16_t new_pad;
        uint16_t bit;

        new_pad = (uint16_t)(pad & ~pad_last & ~pad_suppressed);

        for (bit = 1; bit < 0x1000; bit = (uint16_t)(bit << 1)) {
            if (new_pad & bit)
                add_event(bit);
        }
    }

    pad_last = pad;
    pad_suppressed &= pad;

    return pad;
}


static void mouse_scan(void)
{
    uint8_t port;

    for (port = 0; port < 2; port++) {
        if (controller_is_mouse(port)) {
            mouse_port = port;
            mouse_lost = 0;
            mouse_last_buttons = 0;
            return;
        }
    }
}


static void mouse_os_poll(game_mouse_t* mouse)
{
    uint8_t buffer[5];
    uint16_t size = 5;
    uint16_t held;
    uint16_t pressed;

    if (read(mouse_dev, buffer, &size) != ERR_SUCCESS || size != 5)
        return;

    mouse->present = 1;

    if (buffer[1] != 0 || buffer[2] != 0 ||
        buffer[3] != 0 || buffer[4] != 0) {
        mouse->moved = 1;
        mouse->dx = (int16_t)(buffer[1] | (buffer[2] << 8));
        mouse->dy = (int16_t)(buffer[3] | (buffer[4] << 8));
    }

    held = (uint16_t)(buffer[0] & (MOUSE_BTN_LEFT | MOUSE_BTN_RIGHT));
    pressed = (uint16_t)(held & ~mouse_last_buttons);

    if (pressed & MOUSE_BTN_LEFT)
        mouse->buttons |= BUTTON_A;
    if (pressed & MOUSE_BTN_RIGHT)
        mouse->buttons |= BUTTON_B;

    mouse_last_buttons = held;
}


uint16_t game_input_events(void)
{
    uint16_t events;

    drain_input();
    sample_pads(1);

    events = press_bits;
    press_bits = 0;

    return events;
}


void game_input_mouse(game_mouse_t* mouse)
{
    ControllerMouse report;
    uint8_t flags;
    uint16_t held;
    uint16_t pressed;

    mouse->present = 0;
    mouse->moved = 0;
    mouse->dx = 0;
    mouse->dy = 0;
    mouse->buttons = 0;

    if (mouse_port == MOUSE_PORT_NONE) {
        if (mouse_dev >= 0) {
            mouse_os_poll(mouse);
            return;
        }

        if (++mouse_scan_timer >= MOUSE_SCAN_FRAMES) {
            mouse_scan_timer = 0;
            mouse_scan();

            if (mouse_port == MOUSE_PORT_NONE)
                mouse_dev = open(MOUSE_DEVICE_NAME, O_RDONLY);
        }

        return;
    }

    flags = controller_read_mouse(mouse_port, &report);

    if (flags == MOUSE_ERROR) {
        if (++mouse_lost >= MOUSE_LOST_FRAMES) {
            mouse_port = MOUSE_PORT_NONE;
            mouse_lost = 0;
            mouse_last_buttons = 0;
        }
        return;
    }

    mouse_lost = 0;
    mouse->present = 1;

    if (flags & 3) {
        mouse->moved = 1;
        mouse->dx = report.x;
        mouse->dy = report.y;
    }

    held = (uint16_t)(report.bits & (MOUSE_LEFT | MOUSE_RIGHT));
    pressed = (uint16_t)(held & ~mouse_last_buttons);

    if (pressed & MOUSE_LEFT)
        mouse->buttons |= BUTTON_A;
    if (pressed & MOUSE_RIGHT)
        mouse->buttons |= BUTTON_B;

    mouse_last_buttons = held;
}


void game_input_clear(void)
{
    /* Parse instead of discarding: keeping key_down in sync avoids a release
     * prefix being split across two reads, which would turn the following key
     * code into a phantom press. */
    drain_input();

    press_bits = 0;
    sample_pads(0);
}
