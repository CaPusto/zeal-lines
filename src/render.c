#include <stdint.h>
#include <stddef.h>

#include <zos_errors.h>
#include <zos_vfs.h>
#include <zos_video.h>

#include <zgdk/file.h>
#include <zgdk/hud.h>
#include <zgdk/ascii.h>

#include <zvb_gfx.h>

#include "assets_generated.h"
#include "render.h"

#define TILE_BUFFER_SIZE 2048

static gfx_context video;

#define HUD_TITLE1          0
#define HUD_TITLE1_CAP      4
#define HUD_TITLE2          4
#define HUD_TITLE2_CAP      5
#define HUD_SUBTITLE        9
#define HUD_SUBTITLE_CAP    13
#define HUD_SCORE_LABEL     22
#define HUD_SCORE_LABEL_CAP 5
#define HUD_SCORE           27
#define HUD_SCORE_CAP       5
#define HUD_NEXT_LABEL      32
#define HUD_NEXT_LABEL_CAP  4
#define HUD_PREVIEW         36
#define HUD_PREVIEW_CAP     3
#define HUD_GOAL1           39
#define HUD_GOAL1_CAP       16
#define HUD_GOAL2           55
#define HUD_GOAL2_CAP       16
#define HUD_GOAL3           71
#define HUD_GOAL3_CAP       12
#define HUD_AUTOSAVE        83
#define HUD_AUTOSAVE_CAP    8
#define HUD_NEWGAME         91
#define HUD_NEWGAME_CAP     8
#define HUD_START           99
#define HUD_START_CAP       12
#define HUD_SELECT          111
#define HUD_SELECT_CAP      13
#define HUD_SPR_COUNT       124

/* The HUD "NEW GAME" button occupies these tile rows. */
#define HUD_BUTTON_TILE_Y   22
#define HUD_BUTTON_TILE_H   2

/* The last four hardware sprites draw the board cursor reticle. */
#define HUD_CURSOR          124
#define HUD_CURSOR_COUNT    4

static gfx_sprite hud_sprites[HUD_CURSOR + HUD_CURSOR_COUNT];

/*
 * What is currently on screen for every board cell, so a cursor move only
 * repaints the two cells that actually changed instead of the whole screen.
 * A full board repaint costs ~60 VRAM transfers, which is slow enough to
 * swallow keyboard input on the real machine.
 */
static uint8_t drawn_group[GRID_SIZE * GRID_SIZE];
static uint8_t drawn_size[GRID_SIZE * GRID_SIZE];
static uint8_t drawn_valid;
static uint8_t drawn_cursor_valid;
static uint8_t drawn_cursor_x;
static uint8_t drawn_cursor_y;


static void hud_text(uint8_t offset, uint8_t capacity, uint8_t spacing,
                     const char* text, int16_t x, int16_t y)
{
    hud_sprite_text_t view;

    view.sprites = &hud_sprites[offset];
    view.capacity = capacity;
    view.spacing = spacing;
    hud_sprite_put_s(&view, text, x, y);
}


/* The large title uses 16x16 glyphs from the atlas instead of the 8x8 font,
 * so it is drawn one sprite per letter instead of through ascii_map(). */
static uint8_t title_glyph(char c)
{
    const char* glyphs = ZEAL_TITLE_GLYPHS;
    uint8_t i;

    for (i = 0; glyphs[i] != 0; i++) {
        if (glyphs[i] == c)
            return i;
    }

    return 0;
}


static void hud_large_text(uint8_t offset, uint8_t capacity,
                           const char* text, int16_t x, int16_t y)
{
    uint8_t i;

    for (i = 0; i < capacity; i++) {
        gfx_sprite* sprite = &hud_sprites[offset + i];

        if (text[i] != 0) {
            sprite->tile = (uint8_t)(TILE_TITLE + title_glyph(text[i]));
            sprite->x = (uint16_t)(x + 16 + i * 16);
            sprite->y = (uint16_t)(y + 16);
        } else {
            sprite->y = 512;
        }
    }
}


static void hud_draw_score(const game_t* game)
{
    char buffer[6];
    uint16_t value;
    uint8_t pos;

    value = game->score;
    buffer[5] = 0;
    pos = 5;

    if (value == 0) {
        buffer[4] = '0';
        pos = 4;
    } else {
        while (value != 0 && pos != 0) {
            buffer[--pos] = (char)('0' + (value % 10));
            value /= 10;
        }
    }

    hud_text(HUD_SCORE, HUD_SCORE_CAP, 16, &buffer[pos], 468, 96);
}


static void hud_draw_preview(const game_t* game)
{
    uint8_t i;

    for (i = 0; i < HUD_PREVIEW_CAP; i++) {
        gfx_sprite* sprite;
        uint8_t color;

        sprite = &hud_sprites[HUD_PREVIEW + i];
        color = game->preview[i];

        if (color < 1 || color > 7)
            color = 1;

        sprite->tile = (uint8_t)(TILE_PREVIEW + color - 1);
        sprite->flags = (uint8_t)(SPRITE_TILE_BIT9 |
                                  ((PALETTE_GROUP_BALL + color - 1) << 4));
        sprite->x = (uint16_t)(468 + i * 20 + 16);
        sprite->y = (uint16_t)(140 + 16);
        sprite->options = 0;
    }
}


void render_hud_init(const game_t* game)
{
    uint8_t i;

    for (i = 0; i < HUD_CURSOR + HUD_CURSOR_COUNT; i++) {
        hud_sprites[i].x = 0;
        hud_sprites[i].y = 512;
        hud_sprites[i].tile = 0;
        hud_sprites[i].flags = SPRITE_TILE_BIT9;
        hud_sprites[i].options = 0;
    }

    ascii_map('A', 26, (uint8_t)TILE_FONT_LETTERS);
    ascii_map('0', 10, (uint8_t)TILE_FONT_DIGITS);
    ascii_map('+', 1, (uint8_t)(TILE_FONT_PUNCT + 0));
    ascii_map('/', 1, (uint8_t)(TILE_FONT_PUNCT + 1));
    ascii_map('-', 1, (uint8_t)(TILE_FONT_PUNCT + 2));

    hud_large_text(HUD_TITLE1, HUD_TITLE1_CAP, "ZEAL", 468, 18);
    hud_large_text(HUD_TITLE2, HUD_TITLE2_CAP, "LINES", 468, 35);
    hud_text(HUD_SUBTITLE, HUD_SUBTITLE_CAP, 8, "RETRO EDITION", 468, 58);
    hud_text(HUD_SCORE_LABEL, HUD_SCORE_LABEL_CAP, 8, "SCORE", 468, 82);
    hud_text(HUD_NEXT_LABEL, HUD_NEXT_LABEL_CAP, 8, "NEXT", 468, 122);
    hud_text(HUD_GOAL1, HUD_GOAL1_CAP, 8, "MAKE FIVE+ LINES", 468, 172);
    hud_text(HUD_GOAL2, HUD_GOAL2_CAP, 8, "OR + / X CROSSES", 468, 186);
    hud_text(HUD_GOAL3, HUD_GOAL3_CAP, 8, "OF ONE COLOR", 468, 200);
    hud_text(HUD_AUTOSAVE, HUD_AUTOSAVE_CAP, 8, "AUTOSAVE", 468, 226);
    hud_text(HUD_NEWGAME, HUD_NEWGAME_CAP, 8, "NEW GAME", 512, 360);
    hud_text(HUD_START, HUD_START_CAP, 8, "A - NEW GAME", 468, 400);
    hud_text(HUD_SELECT, HUD_SELECT_CAP, 8, "SELECT - EXIT", 468, 414);

    hud_draw_score(game);
    hud_draw_preview(game);

    gfx_sprite_render_array(&video, 0, hud_sprites, HUD_SPR_COUNT);
}


void render_hud_update(const game_t* game)
{
    hud_draw_score(game);
    hud_draw_preview(game);

    gfx_sprite_render_array(&video, HUD_SCORE,
                            &hud_sprites[HUD_SCORE], HUD_SCORE_CAP);
    gfx_sprite_render_array(&video, HUD_PREVIEW,
                            &hud_sprites[HUD_PREVIEW], HUD_PREVIEW_CAP);
}


static void render_cursor_sprites(uint8_t cx, uint8_t cy)
{
    static const uint8_t offsets[HUD_CURSOR_COUNT][2] = {
        { 0,  0 },
        { 32, 0 },
        { 0,  32 },
        { 32, 32 }
    };

    uint16_t x;
    uint16_t y;
    uint8_t i;

    x = (uint16_t)((GRID_TILE_X + cx * CELL_TILES) * 16 + 16);
    y = (uint16_t)((GRID_TILE_Y + cy * CELL_TILES) * 16 + 16);

    for (i = 0; i < HUD_CURSOR_COUNT; i++) {
        gfx_sprite* sprite = &hud_sprites[HUD_CURSOR + i];

        sprite->tile = (uint8_t)(TILE_CURSOR + i);
        sprite->flags = SPRITE_TILE_BIT9;
        sprite->x = (uint16_t)(x + offsets[i][0]);
        sprite->y = (uint16_t)(y + offsets[i][1]);
        sprite->options = 0;
    }

    gfx_sprite_render_array(&video, HUD_CURSOR,
                            &hud_sprites[HUD_CURSOR], HUD_CURSOR_COUNT);
}


static uint8_t cell_highlighted(const game_t* game,
                                uint8_t cursor_x, uint8_t cursor_y,
                                uint8_t selected, uint8_t sel_x, uint8_t sel_y,
                                const render_overlay_t* overlay,
                                uint8_t x, uint8_t y)
{
    uint8_t index;

    if (x == cursor_x && y == cursor_y)
        return 1;

    if (selected && x == sel_x && y == sel_y)
        return 1;

    if (overlay == NULL)
        return 0;

    index = (uint8_t)(y * GRID_SIZE + x);

    if (game->cells[y][x] == 0 && overlay->path[index])
        return 1;

    if (game->cells[y][x] != 0 && overlay->preview[index] && overlay->blink)
        return 1;

    return 0;
}


static uint8_t cell_group(uint8_t color, uint8_t highlighted)
{
    if (color == 0)
        return highlighted ? PALETTE_GROUP_EMPTY_SELECTED
                           : PALETTE_GROUP_EMPTY;

    return (uint8_t)((highlighted ? PALETTE_GROUP_BALL_SELECTED
                                  : PALETTE_GROUP_BALL) + color - 1);
}


static uint16_t cell_tile_base(uint8_t color)
{
    if (color == 0)
        return TILE_EMPTY;

    return (uint16_t)(TILE_BALL_BASE +
                      (color - 1) * TILE_COLOR_STRIDE);
}


/*
 * Open the tileset, trying the emulator HostFS first and then the paths
 * available on real hardware.
 */
static zos_dev_t open_tileset(void)
{
    zos_dev_t dev;

    dev = open("H:/zeal-tiles.dat", O_RDONLY);
    if (dev >= 0)
        return dev;

    dev = open("zeal-tiles.dat", O_RDONLY);
    if (dev >= 0)
        return dev;

    dev = open("T:/zeal-tiles.dat", O_RDONLY);
    if (dev >= 0)
        return dev;

    return open("A:/zeal-tiles.dat", O_RDONLY);
}


static int load_tileset(void)
{
    static uint8_t buffer[TILE_BUFFER_SIZE];

    zos_dev_t dev;
    zos_err_t err;
    uint32_t offset;
    uint32_t total_size;

    offset = 0;
    total_size = ZEAL_ASSET_FILE_BYTES;

    dev = open_tileset();

    if (dev < 0)
        return 1;

    while (offset < total_size) {
        uint32_t remaining;
        uint16_t amount;
        uint16_t received;

        remaining = total_size - offset;
        amount = (remaining > TILE_BUFFER_SIZE)
               ? TILE_BUFFER_SIZE
               : (uint16_t)remaining;

        received = amount;
        err = file_read_safe(dev, buffer, &received);

        if (err != ERR_SUCCESS ||
            received != amount ||
            received == 0 ||
            (received % ZEAL_ASSET_TILE_BYTES) != 0) {
            close(dev);
            return 1;
        }

        if (gfx_tileset_load_none(&video, buffer, received,
                                  (uint16_t)offset, 0, 0) != GFX_SUCCESS) {
            close(dev);
            return 1;
        }

        offset += received;
    }

    close(dev);
    return 0;
}


int render_video_init(void)
{
    gfx_enable_screen(0);

    if (gfx_initialize(ZVB_CTRL_VID_MODE_GFX_640_4BIT, &video) != GFX_SUCCESS)
        return 1;

    return 0;
}


int render_assets_init(void)
{
    if (gfx_palette_load(&video, (void*)zeal_palette,
                         sizeof(zeal_palette), 0) != GFX_SUCCESS) {
        return 1;
    }

    if (load_tileset() != 0)
        return 1;

    return 0;
}


int render_init(void)
{
    if (render_video_init() != 0)
        return 1;

    return render_assets_init();
}


gfx_context* render_context(void)
{
    return &video;
}


void render_shutdown(void)
{
    ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);
}


void render_board(const game_t* game,
                  uint8_t cursor_x, uint8_t cursor_y,
                  uint8_t selected, uint8_t sel_x, uint8_t sel_y,
                  const render_overlay_t* overlay)
{
    static uint8_t tilemap[SCREEN_TILES_X];
    static uint8_t attributes[SCREEN_TILES_X];

    uint8_t screen_y;
    uint8_t x;

    drawn_valid = 0;

    for (screen_y = 0; screen_y < SCREEN_TILES_Y; screen_y++) {
        for (x = 0; x < SCREEN_TILES_X; x++) {
            tilemap[x] = (uint8_t)TILE_BACKGROUND;
            attributes[x] = 1;
        }

        if (screen_y >= PANEL_TILE_Y &&
            screen_y < PANEL_TILE_Y + PANEL_TILE_H) {
            for (x = PANEL_TILE_X; x < PANEL_TILE_X + PANEL_TILE_W; x++) {
                tilemap[x] = (uint8_t)TILE_PANEL;
                attributes[x] = 1;
            }
        }

        if (screen_y >= HUD_BUTTON_TILE_Y &&
            screen_y < HUD_BUTTON_TILE_Y + HUD_BUTTON_TILE_H) {
            for (x = PANEL_TILE_X; x < PANEL_TILE_X + PANEL_TILE_W; x++) {
                tilemap[x] = (uint8_t)TILE_BUTTON;
                attributes[x] = 1;
            }
        }

        if (screen_y >= GRID_TILE_Y &&
            screen_y < GRID_TILE_Y + GRID_SIZE * CELL_TILES) {

            uint8_t cell_y;
            uint8_t local_y;
            uint8_t cell_x;

            cell_y = (uint8_t)((screen_y - GRID_TILE_Y) / CELL_TILES);
            local_y = (uint8_t)((screen_y - GRID_TILE_Y) % CELL_TILES);

            for (cell_x = 0; cell_x < GRID_SIZE; cell_x++) {
                uint8_t color;
                uint8_t highlighted;
                uint8_t group;
                uint8_t size;
                uint16_t base;
                uint8_t local_x;

                color = game->cells[cell_y][cell_x];
                highlighted = cell_highlighted(game,
                                               cursor_x, cursor_y,
                                               selected, sel_x, sel_y,
                                               overlay, cell_x, cell_y);
                group = cell_group(color, highlighted);

                size = 0;
                if (color != 0 && selected &&
                    cell_x == sel_x && cell_y == sel_y &&
                    overlay != NULL && overlay->pulse) {
                    size = 1;
                }

                base = (uint16_t)(cell_tile_base(color) +
                                  size * TILE_SIZE_STRIDE);

                drawn_group[cell_y * GRID_SIZE + cell_x] = group;
                drawn_size[cell_y * GRID_SIZE + cell_x] = size;

                for (local_x = 0; local_x < CELL_TILES; local_x++) {
                    uint16_t tile_index;
                    uint8_t screen_x;

                    tile_index = (uint16_t)(base +
                                            local_y * CELL_TILES + local_x);
                    screen_x = (uint8_t)(GRID_TILE_X +
                                         cell_x * CELL_TILES + local_x);

                    if (screen_x >= SCREEN_TILES_X)
                        continue;

                    tilemap[screen_x] = (uint8_t)tile_index;
                    attributes[screen_x] = (uint8_t)((group << 4) |
                                                     ((tile_index >> 8) & 1));
                }
            }
        }

        if (gfx_tilemap_load(&video, tilemap, sizeof(tilemap),
                             0, 0, screen_y) != GFX_SUCCESS) {
            return;
        }

        if (gfx_tilemap_load(&video, attributes, sizeof(attributes),
                             1, 0, screen_y) != GFX_SUCCESS) {
            return;
        }
    }

    render_cursor_sprites(cursor_x, cursor_y);

    drawn_valid = 1;
    drawn_cursor_valid = 1;
    drawn_cursor_x = cursor_x;
    drawn_cursor_y = cursor_y;
}


void render_cell_size(const game_t* game, uint8_t cx, uint8_t cy,
                      uint8_t size, uint8_t highlighted)
{
    static uint8_t tiles[CELL_TILES];
    static uint8_t attrs[CELL_TILES];

    uint8_t color;
    uint8_t group;
    uint16_t base;
    uint8_t ly;

    color = game->cells[cy][cx];
    group = cell_group(color, highlighted);
    base = (uint16_t)(cell_tile_base(color) +
                      (color != 0 ? size * TILE_SIZE_STRIDE : 0));

    for (ly = 0; ly < CELL_TILES; ly++) {
        uint8_t lx;

        for (lx = 0; lx < CELL_TILES; lx++) {
            uint16_t tile_index;

            tile_index = (uint16_t)(base + ly * CELL_TILES + lx);

            tiles[lx] = (uint8_t)tile_index;
            attrs[lx] = (uint8_t)((group << 4) | ((tile_index >> 8) & 1));
        }

        gfx_tilemap_load(&video, tiles, CELL_TILES, 0,
                         (uint8_t)(GRID_TILE_X + cx * CELL_TILES),
                         (uint8_t)(GRID_TILE_Y + cy * CELL_TILES + ly));

        gfx_tilemap_load(&video, attrs, CELL_TILES, 1,
                         (uint8_t)(GRID_TILE_X + cx * CELL_TILES),
                         (uint8_t)(GRID_TILE_Y + cy * CELL_TILES + ly));
    }
}


void render_cell(const game_t* game,
                 uint8_t cursor_x, uint8_t cursor_y,
                 uint8_t selected, uint8_t sel_x, uint8_t sel_y,
                 const render_overlay_t* overlay,
                 uint8_t cx, uint8_t cy)
{
    uint8_t color;
    uint8_t highlighted;
    uint8_t size;

    color = game->cells[cy][cx];
    highlighted = cell_highlighted(game, cursor_x, cursor_y,
                                   selected, sel_x, sel_y,
                                   overlay, cx, cy);

    size = 0;
    if (color != 0 && selected && cx == sel_x && cy == sel_y &&
        overlay != NULL && overlay->pulse) {
        size = 1;
    }

    render_cell_size(game, cx, cy, size, highlighted);

    /* The cell was painted directly; force the next diff to repaint it. */
    drawn_group[cy * GRID_SIZE + cx] = 0xFF;
}


/*
 * Repaint a horizontal run of cells in one cell row. One VRAM transfer per
 * screen row instead of one per cell keeps a cursor move cheap.
 */
static void draw_cell_span(const game_t* game, uint8_t cy,
                           uint8_t x0, uint8_t x1)
{
    static uint8_t tiles[GRID_SIZE * CELL_TILES];
    static uint8_t attrs[GRID_SIZE * CELL_TILES];

    uint8_t ly;
    uint8_t cx;

    for (ly = 0; ly < CELL_TILES; ly++) {
        uint8_t count = 0;

        for (cx = x0; cx <= x1; cx++) {
            uint8_t index = (uint8_t)(cy * GRID_SIZE + cx);
            uint8_t color = game->cells[cy][cx];
            uint8_t size = drawn_size[index];
            uint8_t group = drawn_group[index];
            uint16_t base;
            uint8_t lx;

            base = (uint16_t)(cell_tile_base(color) +
                              (color != 0 ? size * TILE_SIZE_STRIDE : 0));

            for (lx = 0; lx < CELL_TILES; lx++) {
                uint16_t tile = (uint16_t)(base + ly * CELL_TILES + lx);

                tiles[count] = (uint8_t)tile;
                attrs[count] = (uint8_t)((group << 4) | ((tile >> 8) & 1));
                count++;
            }
        }

        gfx_tilemap_load(&video, tiles, count, 0,
                         (uint8_t)(GRID_TILE_X + x0 * CELL_TILES),
                         (uint8_t)(GRID_TILE_Y + cy * CELL_TILES + ly));
        gfx_tilemap_load(&video, attrs, count, 1,
                         (uint8_t)(GRID_TILE_X + x0 * CELL_TILES),
                         (uint8_t)(GRID_TILE_Y + cy * CELL_TILES + ly));
    }
}


void render_update(const game_t* game,
                   uint8_t cursor_x, uint8_t cursor_y,
                   uint8_t selected, uint8_t sel_x, uint8_t sel_y,
                   const render_overlay_t* overlay)
{
    static uint8_t next_group[GRID_SIZE * GRID_SIZE];
    static uint8_t next_size[GRID_SIZE * GRID_SIZE];
    uint8_t min_x[GRID_SIZE];
    uint8_t max_x[GRID_SIZE];
    uint8_t any[GRID_SIZE];
    uint8_t x;
    uint8_t y;

    if (!drawn_valid) {
        render_board(game, cursor_x, cursor_y,
                     selected, sel_x, sel_y, overlay);
        return;
    }

    for (y = 0; y < GRID_SIZE; y++) {
        any[y] = 0;
        min_x[y] = 0;
        max_x[y] = 0;
    }

    for (y = 0; y < GRID_SIZE; y++) {
        for (x = 0; x < GRID_SIZE; x++) {
            uint8_t index = (uint8_t)(y * GRID_SIZE + x);
            uint8_t color = game->cells[y][x];
            uint8_t highlighted;
            uint8_t group;
            uint8_t size;

            highlighted = cell_highlighted(game, cursor_x, cursor_y,
                                           selected, sel_x, sel_y,
                                           overlay, x, y);
            group = cell_group(color, highlighted);

            size = 0;
            if (color != 0 && selected && x == sel_x && y == sel_y &&
                overlay != NULL && overlay->pulse) {
                size = 1;
            }

            next_group[index] = group;
            next_size[index] = size;

            if (drawn_group[index] != group ||
                drawn_size[index] != size) {
                if (!any[y]) {
                    any[y] = 1;
                    min_x[y] = x;
                    max_x[y] = x;
                } else {
                    if (x < min_x[y])
                        min_x[y] = x;
                    if (x > max_x[y])
                        max_x[y] = x;
                }
            }
        }
    }

    for (y = 0; y < GRID_SIZE; y++) {
        if (!any[y])
            continue;

        for (x = min_x[y]; x <= max_x[y]; x++) {
            uint8_t index = (uint8_t)(y * GRID_SIZE + x);
            drawn_group[index] = next_group[index];
            drawn_size[index] = next_size[index];
        }

        draw_cell_span(game, y, min_x[y], max_x[y]);
    }

    if (!drawn_cursor_valid ||
        cursor_x != drawn_cursor_x || cursor_y != drawn_cursor_y) {
        render_cursor_sprites(cursor_x, cursor_y);
        drawn_cursor_valid = 1;
        drawn_cursor_x = cursor_x;
        drawn_cursor_y = cursor_y;
    }
}


void render_invalidate(void)
{
    drawn_valid = 0;
    drawn_cursor_valid = 0;
}


uint8_t render_board_hit(uint16_t x, uint16_t y)
{
    uint16_t left = (uint16_t)(GRID_TILE_X * 16);
    uint16_t top = (uint16_t)(GRID_TILE_Y * 16);
    uint16_t size = (uint16_t)(GRID_SIZE * CELL_TILES * 16);

    return (uint8_t)(x >= left && x < left + size &&
                     y >= top && y < top + size);
}


uint8_t render_hud_button_hit(uint16_t x, uint16_t y)
{
    uint16_t left = (uint16_t)(PANEL_TILE_X * 16);
    uint16_t top = (uint16_t)(HUD_BUTTON_TILE_Y * 16);
    uint16_t width = (uint16_t)(PANEL_TILE_W * 16);
    uint16_t height = (uint16_t)(HUD_BUTTON_TILE_H * 16);

    return (uint8_t)(x >= left && x < left + width &&
                     y >= top && y < top + height);
}


void render_hud_hide(void)
{
    uint8_t i;

    for (i = 0; i < HUD_CURSOR + HUD_CURSOR_COUNT; i++)
        hud_sprites[i].y = 512;

    gfx_sprite_render_array(&video, 0, hud_sprites,
                            (uint8_t)(HUD_CURSOR + HUD_CURSOR_COUNT));
}


static void fill_run(uint8_t x, uint8_t y, uint8_t width, uint16_t tile)
{
    static uint8_t tiles[SCREEN_TILES_X];
    static uint8_t attrs[SCREEN_TILES_X];
    uint8_t i;

    for (i = 0; i < width; i++) {
        tiles[i] = (uint8_t)tile;
        attrs[i] = (uint8_t)((tile >> 8) & 1);
    }

    gfx_tilemap_load(&video, tiles, width, 0, x, y);
    gfx_tilemap_load(&video, attrs, width, 1, x, y);
}


void render_screen_fill(uint16_t tile)
{
    uint8_t y;

    for (y = 0; y < SCREEN_TILES_Y; y++)
        fill_run(0, y, SCREEN_TILES_X, tile);
}


void render_box(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t tile)
{
    uint8_t row;

    for (row = 0; row < h; row++)
        fill_run(x, (uint8_t)(y + row), w, tile);
}


static uint8_t overlay_sprite;


void render_sprites_begin(void)
{
    render_hud_hide();
    overlay_sprite = 0;
}


void render_sprites_text(int16_t x, int16_t y, const char* text)
{
    render_sprites_text_color(x, y, text, 8, 0);
}


void render_sprites_text_spaced(int16_t x, int16_t y, const char* text,
                                uint8_t spacing)
{
    render_sprites_text_color(x, y, text, spacing, 0);
}


void render_sprites_text_color(int16_t x, int16_t y, const char* text,
                               uint8_t spacing, uint8_t palette)
{
    hud_sprite_text_t view;
    uint8_t length = 0;
    uint8_t i;

    while (text[length] != 0)
        length++;

    if (length > HUD_SPR_COUNT - overlay_sprite)
        length = (uint8_t)(HUD_SPR_COUNT - overlay_sprite);

    if (length == 0)
        return;

    view.sprites = &hud_sprites[overlay_sprite];
    view.capacity = length;
    view.spacing = spacing;
    hud_sprite_put_s(&view, text, x, y);

    for (i = 0; i < length; i++) {
        gfx_sprite* sprite = &hud_sprites[overlay_sprite + i];

        sprite->flags = (uint8_t)(SPRITE_TILE_BIT9 | (palette << 4));
    }

    gfx_sprite_render_array(&video, overlay_sprite,
                            &hud_sprites[overlay_sprite], length);

    overlay_sprite = (uint8_t)(overlay_sprite + length);
}


void render_sprites_tile(int16_t x, int16_t y, uint16_t tile, uint8_t palette)
{
    gfx_sprite* sprite;

    if (overlay_sprite >= HUD_SPR_COUNT)
        return;

    sprite = &hud_sprites[overlay_sprite];
    sprite->x = (uint16_t)(x + 16);
    sprite->y = (uint16_t)(y + 16);
    sprite->tile = (uint8_t)tile;
    sprite->flags = (uint8_t)((tile & 0x100 ? SPRITE_TILE_BIT9 : 0) |
                              (palette << 4));
    sprite->options = 0;

    gfx_sprite_render_array(&video, overlay_sprite, sprite, 1);

    overlay_sprite = (uint8_t)(overlay_sprite + 1);
}



void render_wait_frame(void)
{
    gfx_wait_vblank(&video);
    gfx_wait_end_vblank(&video);
}
