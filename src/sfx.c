#include <stdint.h>

#include <zvb_sound.h>

#include "sfx.h"

/*
 * VOICE0 - UI click / move tick
 * VOICE1 - clear/collapse
 * VOICE2 - spawn / new game
 * VOICE3 - game over jingle
 */
#define SND_UI      VOICE0
#define SND_CLEAR   VOICE1
#define SND_SPAWN   VOICE2
#define SND_JINGLE  VOICE3
#define SND_ALL     (sound_voice_t)(VOICE0 | VOICE1 | VOICE2 | VOICE3)

#define SND_CLICK_WAVE  (sound_waveform_t)(WAV_SQUARE | DUTY_CYCLE_12_5)
#define SND_BLIP_WAVE   (sound_waveform_t)(WAV_SQUARE | DUTY_CYCLE_25_0)
#define SND_BELL_WAVE   WAV_TRIANGLE

typedef struct {
    uint16_t divider;
    uint8_t frames;
} sfx_note_t;

static const sfx_note_t game_over_notes[] = {
    { SOUND_FREQ_TO_DIV(330), 12 },
    { SOUND_FREQ_TO_DIV(294), 12 },
    { SOUND_FREQ_TO_DIV(262), 24 },
};

static const sfx_note_t new_game_notes[] = {
    { SOUND_FREQ_TO_DIV(523), 6 },
    { SOUND_FREQ_TO_DIV(659), 6 },
    { SOUND_FREQ_TO_DIV(784), 10 },
};

static const sfx_note_t* melody;
static uint8_t melody_length;
static uint8_t melody_position;
static uint8_t melody_frames;
static uint8_t melody_wave;

static uint8_t ui_timer;
static uint8_t clear_timer;
static uint8_t clear_stage;
static uint8_t spawn_timer;
static uint8_t spawn_stage;
static uint8_t saved_page;

static void sound_begin(void)
{
    saved_page = zvb_config_dev_idx;
}

static void sound_end(void)
{
    zvb_config_dev_idx = saved_page;
}

static void melody_next(void)
{
    const sfx_note_t* note;

    if (melody == 0 || melody_position >= melody_length) {
        melody = 0;
        zvb_sound_set_hold(SND_JINGLE, 1);
        return;
    }

    note = &melody[melody_position++];
    zvb_sound_set_voices(SND_JINGLE, note->divider,
                         (sound_waveform_t)melody_wave);
    melody_frames = note->frames;
}

static void melody_start(const sfx_note_t* notes, uint8_t length,
                         uint8_t wave)
{
    melody = notes;
    melody_length = length;
    melody_position = 0;
    melody_frames = 0;
    melody_wave = wave;

    zvb_sound_set_hold(SND_JINGLE, 1);
    melody_next();
}

void zsfx_init(void)
{
    sound_begin();
    zvb_sound_initialize(1);
    zvb_sound_set_channels(SND_ALL, SND_ALL);
    zvb_sound_set_volume(VOL_75);
    zvb_sound_set_voices_vol(SND_UI, VOL_50);
    zvb_sound_set_voices_vol(SND_CLEAR, VOL_75);
    zvb_sound_set_voices_vol(SND_SPAWN, VOL_50);
    zvb_sound_set_voices_vol(SND_JINGLE, VOL_75);
    zvb_sound_set_hold(SND_ALL, 1);
    sound_end();

    ui_timer = 0;
    clear_timer = 0;
    clear_stage = 0;
    spawn_timer = 0;
    spawn_stage = 0;
    melody = 0;
    melody_frames = 0;
}

void zsfx_update(void)
{
    sound_begin();

    if (ui_timer != 0) {
        ui_timer--;
        if (ui_timer == 0)
            zvb_sound_set_hold(SND_UI, 1);
    }

    if (clear_timer != 0) {
        clear_timer--;
        if (clear_timer == 0) {
            if (clear_stage == 0) {
                clear_stage = 1;
                clear_timer = 10;
                zvb_sound_set_voices(SND_CLEAR,
                                     SOUND_FREQ_TO_DIV(2093), SND_BELL_WAVE);
            } else {
                zvb_sound_set_hold(SND_CLEAR, 1);
            }
        }
    }

    if (spawn_timer != 0) {
        spawn_timer--;
        if (spawn_timer == 0) {
            if (spawn_stage == 0) {
                spawn_stage = 1;
                spawn_timer = 4;
                zvb_sound_set_voices(SND_SPAWN,
                                     SOUND_FREQ_TO_DIV(880), SND_BLIP_WAVE);
            } else {
                zvb_sound_set_hold(SND_SPAWN, 1);
            }
        }
    }

    if (melody != 0 && melody_frames != 0) {
        melody_frames--;
        if (melody_frames == 0)
            melody_next();
    }

    sound_end();
}

void zsfx_stop_all(void)
{
    sound_begin();
    zvb_sound_set_hold(SND_ALL, 1);
    sound_end();

    ui_timer = 0;
    clear_timer = 0;
    clear_stage = 0;
    spawn_timer = 0;
    spawn_stage = 0;
    melody = 0;
    melody_frames = 0;
}

void zsfx_select(void)
{
    sound_begin();
    zvb_sound_set_voices(SND_UI, SOUND_FREQ_TO_DIV(1500), SND_CLICK_WAVE);
    sound_end();
    ui_timer = 2;
}

void zsfx_move(void)
{
    sound_begin();
    zvb_sound_set_voices(SND_UI, SOUND_FREQ_TO_DIV(900), SND_CLICK_WAVE);
    sound_end();
    ui_timer = 2;
}

void zsfx_spawn(void)
{
    sound_begin();
    zvb_sound_set_voices(SND_SPAWN, SOUND_FREQ_TO_DIV(440), SND_BLIP_WAVE);
    sound_end();
    spawn_stage = 0;
    spawn_timer = 3;
}

void zsfx_clear(uint8_t count)
{
    uint16_t divider;

    if (count > 6)
        divider = SOUND_FREQ_TO_DIV(1568);
    else
        divider = SOUND_FREQ_TO_DIV(1047);

    sound_begin();
    zvb_sound_set_voices(SND_CLEAR, divider, SND_BELL_WAVE);
    sound_end();

    clear_stage = 0;
    clear_timer = 4;
}

void zsfx_game_over(void)
{
    sound_begin();
    melody_start(game_over_notes,
                 (uint8_t)(sizeof(game_over_notes) / sizeof(game_over_notes[0])),
                 WAV_TRIANGLE);
    sound_end();
}

void zsfx_new_game(void)
{
    sound_begin();
    melody_start(new_game_notes,
                 (uint8_t)(sizeof(new_game_notes) / sizeof(new_game_notes[0])),
                 WAV_SQUARE | DUTY_CYCLE_25_0);
    sound_end();
}
