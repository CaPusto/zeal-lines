#pragma once

/*
 * Seed the ZGDK random generator with the hardware entropy available on the
 * Zeal 8-bit Computer: RTC date/time, the millisecond timer when present, and
 * several samples of the Z80 refresh register.
 */
void entropy_seed_random(void);
