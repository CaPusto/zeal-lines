#include <stdint.h>

#include <zos_errors.h>
#include <zos_time.h>

#include <zgdk/utils.h>

#include "entropy.h"

/*
 * There is no dedicated TRNG on the Zeal 8-bit Computer.  Use the hardware
 * sources that change between runs:
 *
 * - DS1307 RTC date and time;
 * - the millisecond timer, when the target implements it;
 * - the Z80 refresh register, sampled several times.
 *
 * The values are mixed into a non-zero 16-bit seed for the ZGDK LFSR.
 */
static uint16_t mix_byte(uint16_t seed, uint8_t value)
{
    seed ^= value;
    seed = (uint16_t)((seed << 5) | (seed >> 11));
    seed = (uint16_t)(seed + 0x9E37);
    return seed;
}


void entropy_seed_random(void)
{
    zos_date_t date;
    zos_time_t time;
    uint16_t seed;
    uint8_t i;

    seed = 0x5EED;

    if (getdate(&date) == ERR_SUCCESS) {
        seed = mix_byte(seed, date.d_seconds);
        seed = mix_byte(seed, date.d_minutes);
        seed = mix_byte(seed, date.d_hours);
        seed = mix_byte(seed, date.d_day);
        seed = mix_byte(seed, date.d_month);
        seed = mix_byte(seed, date.d_year_l);
        seed = mix_byte(seed, date.d_year_h);
        seed = mix_byte(seed, date.d_date);
    }

    if (gettime(0, &time) == ERR_SUCCESS) {
        seed = mix_byte(seed, (uint8_t)(time.t_millis & 0xFF));
        seed = mix_byte(seed, (uint8_t)(time.t_millis >> 8));
    }

    for (i = 0; i < 16; i++)
        seed = mix_byte(seed, (uint8_t)rand8_quick());

    if (seed == 0)
        seed = 0x5EED;

    rand8_seed(seed);
}
