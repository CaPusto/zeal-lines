#pragma once

#include <stdint.h>

#include <zvb_gfx.h>

/*
 * Show the ready-made title screen from assets/splash.ztp, splash.zts and
 * splash.ztm: load palette, tileset and tilemap, fade in from black, wait for
 * a key press or 25 seconds, then fade out to black.
 *
 * Returns 1 when the splash was shown, 0 when an asset is missing or cannot be
 * read. The caller keeps using its own palette and tileset afterwards, so the
 * game assets have to be (re)loaded after this call.
 */
uint8_t splash_show(gfx_context* ctx);
