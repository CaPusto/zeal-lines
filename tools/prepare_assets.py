#!/usr/bin/env python3
"""
Generate the Zeal-Lines tileset and palette.

Output layout (4bpp, 512 tiles, 65,536 bytes):

    0..8       empty cell (9 tiles)
    9..449     7 colors x 7 sizes x 9 tiles (441 tiles)
    450..475   English letters A-Z, 8x8 inside 16x16 tiles
    476..478   punctuation + / -
    479..488   score digits 0-9, 10x14 inside 16x16 tiles
    489..495   preview mini-balls 16x16 (one per color)
    496        solid background tile (palette index 0)
    497        solid panel tile (palette index 6)
    498        solid button tile (palette index 3)
    499        solid black tile (palette index 7)
    500..503   cursor reticle corners (transparent outside the bracket)
    504..510   large title letters Z E A L I N S (10x14 inside 16x16)
    511        reserve

Palette groups:

    0          empty cell normal
    1          empty cell selected
    2..8       colors 1..7 normal
    9..15      colors 1..7 selected

Inside every group indices 0..6 are the cell background/border and 7..15 are
the ball shades.  Group 0/1 index 15 is white for the sprite HUD font.
"""

from pathlib import Path
from urllib.request import urlopen
from io import BytesIO

from PIL import Image, ImageChops, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
APP = "https://raw.githubusercontent.com/CaPusto/L2/main/src/app/"
COLORS = ["yellow", "green", "red", "blue", "violet", "aqua", "pink"]

CELL_SIZE = 48
BALL_SIZE = 32
TILE_SIZE = 16
TILE_BYTES = 128
TILES_PER_CELL = 9
FRAMES = 7
VARIANTS = 1 + len(COLORS) * 7
FRAME_BYTES = FRAMES * TILES_PER_CELL * TILE_BYTES

TOTAL_TILES = 512
TILE_EMPTY = 0
TILE_BALL_BASE = 9
TILE_COLOR_STRIDE = 7 * 9
TILE_SIZE_STRIDE = 9
TILE_FONT_LETTERS = 450
TILE_FONT_PUNCT = 476
TILE_FONT_DIGITS = 479
TILE_PREVIEW = 489
TILE_BACKGROUND = 496
TILE_PANEL = 497
TILE_BUTTON = 498
TILE_BLACK = 499
TILE_CURSOR = 500
TILE_TITLE = 504
TILE_RESERVE = 511

# Unique letters of "ZEAL LINES", in atlas order.
TITLE_GLYPHS = "ZEALINS"

# Cell background ramps.  Index 0 is the lightest, index 6 the darkest.
NORMAL_RAMP = [114, 105, 96, 87, 78, 69, 60]
SELECTED_RAMP = [150, 141, 132, 123, 114, 105, 96]
WHITE = (255, 255, 255)
BLACK = (0, 0, 0)


def fetch_image(name):
    with urlopen(APP + name, timeout=30) as response:
        data = response.read()
    return Image.open(BytesIO(data)).convert("RGBA")


def crop_frame(sheet, frame):
    y = frame * 60
    frame_image = Image.new("RGBA", (55, 55), (0, 0, 0, 0))
    part = sheet.crop((0, y, 55, min(y + 55, sheet.height)))
    frame_image.alpha_composite(part, (0, 0))
    return frame_image


def rgb565(rgb):
    r, g, b = rgb[:3]
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def closest_index(rgb, palette):
    r, g, b = rgb[:3]
    best = 0
    best_d = 1 << 30
    for i, (pr, pg, pb) in enumerate(palette):
        d = (r - pr) ** 2 + (g - pg) ** 2 + (b - pb) ** 2
        if d < best_d:
            best_d = d
            best = i
    return best


def nearest_ramp_image(image, ramp):
    gray = image.convert("L")
    out = Image.new("P", image.size)
    palette = []
    for value in ramp:
        palette.extend([value, value, value])
    out.putpalette(palette + [0] * (768 - len(palette)))
    src = gray.load()
    dst = out.load()
    for y in range(image.height):
        for x in range(image.width):
            value = src[x, y]
            best = 0
            best_d = 1 << 30
            for i, level in enumerate(ramp):
                d = abs(value - level)
                if d < best_d:
                    best_d = d
                    best = i
            dst[x, y] = best
    return out


def ball_palette(sheet):
    pixels = []
    for frame in range(FRAMES):
        part = crop_frame(sheet, frame)
        for r, g, b, a in part.getdata():
            if a >= 128:
                pixels.append((r, g, b))
    strip = Image.new("RGB", (len(pixels), 1))
    strip.putdata(pixels)
    quantized = strip.quantize(colors=9, method=Image.MEDIANCUT,
                               dither=Image.NONE)
    raw = quantized.getpalette()[:27]
    colors = [tuple(raw[i:i + 3]) for i in range(0, 27, 3)]
    colors.sort(key=lambda c: c[0] + c[1] + c[2])
    return colors


def palette_image(colors):
    image = Image.new("P", (1, 1))
    flat = []
    for color in colors:
        flat.extend(color[:3])
    image.putpalette(flat + [0] * (768 - len(flat)))
    return image


def compose_ball_cell(cell, ball, ball_colors, cell_ramp):
    base = cell.convert("RGBA")
    composed = Image.new("RGBA", (CELL_SIZE, CELL_SIZE), (0, 0, 0, 0))
    composed.alpha_composite(base, (0, 0))
    composed.alpha_composite(ball, ((CELL_SIZE - BALL_SIZE) // 2,
                                    (CELL_SIZE - BALL_SIZE) // 2))
    palette = [(v, v, v) for v in cell_ramp] + ball_colors
    return composed.convert("RGB").quantize(
        palette=palette_image(palette),
        dither=Image.NONE,
    )


def pack_image(image, width, height):
    """Pack a P-mode image into 4bpp bytes, row by row."""
    pixels = image.load()
    result = bytearray()
    for y in range(height):
        for x in range(0, width, 2):
            left = pixels[x, y] & 15
            right = pixels[x + 1, y] & 15
            result.append((left << 4) | right)
    return result


def image_tiles(indexed):
    pixels = indexed.load()
    result = bytearray()
    for ty in range(3):
        for tx in range(3):
            for y in range(TILE_SIZE):
                for x in range(0, TILE_SIZE, 2):
                    left = pixels[tx * TILE_SIZE + x,
                                  ty * TILE_SIZE + y] & 15
                    right = pixels[tx * TILE_SIZE + x + 1,
                                   ty * TILE_SIZE + y] & 15
                    result.append((left << 4) | right)
    assert len(result) == TILES_PER_CELL * TILE_BYTES
    return result


def preview_tile(ball, colors):
    """16x16 preview ball tile: index 0 transparent, 7..15 ball shades."""
    small = ball.resize((TILE_SIZE, TILE_SIZE), Image.LANCZOS)
    out = Image.new("P", (TILE_SIZE, TILE_SIZE), 0)
    palette = []
    for value in NORMAL_RAMP:
        palette.extend([value, value, value])
    for color in colors:
        palette.extend(color)
    out.putpalette(palette + [0] * (768 - len(palette)))
    dst = out.load()
    src = small.load()
    for y in range(TILE_SIZE):
        for x in range(TILE_SIZE):
            r, g, b, a = src[x, y]
            if a < 128:
                dst[x, y] = 0
            else:
                dst[x, y] = 7 + closest_index((r, g, b), colors)
    return pack_image(out, TILE_SIZE, TILE_SIZE)


def solid_tile(index):
    image = Image.new("P", (TILE_SIZE, TILE_SIZE), index)
    return pack_image(image, TILE_SIZE, TILE_SIZE)


def cursor_tile(corner):
    """16x16 transparent tile with a white corner bracket.

    Four of them are drawn as sprites over the cursor cell: the bracket shape
    stays readable on empty cells and on top of any ball color, unlike a
    palette-only highlight.
    """
    image = Image.new("P", (TILE_SIZE, TILE_SIZE), 0)
    dst = image.load()
    arm = 10
    thick = 3

    for y in range(TILE_SIZE):
        for x in range(TILE_SIZE):
            near_left = x < arm
            near_right = x >= TILE_SIZE - arm
            near_top = y < arm
            near_bottom = y >= TILE_SIZE - arm
            on_left = x < thick
            on_right = x >= TILE_SIZE - thick
            on_top = y < thick
            on_bottom = y >= TILE_SIZE - thick

            if corner == 0:
                ink = (on_top and near_left) or (on_left and near_top)
            elif corner == 1:
                ink = (on_top and near_right) or (on_right and near_top)
            elif corner == 2:
                ink = (on_bottom and near_left) or (on_left and near_bottom)
            else:
                ink = (on_bottom and near_right) or (on_right and near_bottom)

            if ink:
                dst[x, y] = 15

    return pack_image(image, TILE_SIZE, TILE_SIZE)


# 5x7 pixel font.  '#' is ink, '.' is background.
FONT = {
    'A': [".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"],
    'B': ["####.", "#...#", "#...#", "####.", "#...#", "#...#", "####."],
    'C': [".###.", "#...#", "#....", "#....", "#....", "#...#", ".###."],
    'D': ["####.", "#...#", "#...#", "#...#", "#...#", "#...#", "####."],
    'E': ["#####", "#....", "#....", "####.", "#....", "#....", "#####"],
    'F': ["#####", "#....", "#....", "####.", "#....", "#....", "#...."],
    'G': [".###.", "#...#", "#....", "#.###", "#...#", "#...#", ".###."],
    'H': ["#...#", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"],
    'I': ["#####", "..#..", "..#..", "..#..", "..#..", "..#..", "#####"],
    'J': ["..###", "...#.", "...#.", "...#.", "#..#.", "#..#.", ".##.."],
    'K': ["#...#", "#..#.", "#.#..", "##...", "#.#..", "#..#.", "#...#"],
    'L': ["#....", "#....", "#....", "#....", "#....", "#....", "#####"],
    'M': ["#...#", "##.##", "#.#.#", "#...#", "#...#", "#...#", "#...#"],
    'N': ["#...#", "##..#", "#.#.#", "#..##", "#...#", "#...#", "#...#"],
    'O': [".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."],
    'P': ["####.", "#...#", "#...#", "####.", "#....", "#....", "#...."],
    'Q': [".###.", "#...#", "#...#", "#...#", "#.#.#", "#..#.", ".##.#"],
    'R': ["####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#"],
    'S': [".####", "#....", "#....", ".###.", "....#", "....#", "####."],
    'T': ["#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.."],
    'U': ["#...#", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."],
    'V': ["#...#", "#...#", "#...#", "#...#", "#...#", ".#.#.", "..#.."],
    'W': ["#...#", "#...#", "#...#", "#...#", "#.#.#", "##.##", "#...#"],
    'X': ["#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#"],
    'Y': ["#...#", "#...#", ".#.#.", "..#..", "..#..", "..#..", "..#.."],
    'Z': ["#####", "....#", "...#.", "..#..", ".#...", "#....", "#####"],
    '0': [".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."],
    '1': ["..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###."],
    '2': [".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####"],
    '3': ["####.", "....#", "....#", ".###.", "....#", "....#", "####."],
    '4': ["...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#."],
    '5': ["#####", "#....", "####.", "....#", "....#", "#...#", ".###."],
    '6': [".###.", "#....", "#....", "####.", "#...#", "#...#", ".###."],
    '7': ["#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..."],
    '8': [".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###."],
    '9': [".###.", "#...#", "#...#", ".####", "....#", "....#", ".###."],
    '+': [".....", "..#..", "..#..", "#####", "..#..", "..#..", "....."],
    '/': ["....#", "...#.", "...#.", "..#..", ".#...", ".#...", "#...."],
    '-': [".....", ".....", ".....", "#####", ".....", ".....", "....."],
    '.': [".....", ".....", ".....", ".....", ".....", ".##..", ".##.."],
    ':': [".....", ".##..", ".##..", ".....", ".##..", ".##..", "....."],
    "'": ["..#..", "..#..", ".....", ".....", ".....", ".....", "....."],
}

PUNCTUATION = "+/-"


def glyph_tile(glyph, scale, offset_x, offset_y):
    image = Image.new("P", (TILE_SIZE, TILE_SIZE), 0)
    dst = image.load()
    for y, row in enumerate(FONT[glyph]):
        assert len(row) == 5
        for x, ch in enumerate(row):
            if ch != '#':
                continue
            if scale == 1:
                dst[offset_x + x, offset_y + y] = 15
            else:
                for sy in range(scale):
                    for sx in range(scale):
                        dst[offset_x + x * scale + sx,
                            offset_y + y * scale + sy] = 15
    return pack_image(image, TILE_SIZE, TILE_SIZE)


def main():
    (ROOT / "assets").mkdir(exist_ok=True)
    (ROOT / "src").mkdir(exist_ok=True)

    cell_sheet = fetch_image("cell-bgr.png")
    if cell_sheet.size != (69, 138):
        raise ValueError(f"Unexpected cell-bgr.png size: {cell_sheet.size}")

    normal_cell = cell_sheet.crop((0, 0, 69, 69)).resize(
        (CELL_SIZE, CELL_SIZE), Image.LANCZOS).convert("RGB")
    indexed_cell = nearest_ramp_image(normal_cell, NORMAL_RAMP)

    balls = []
    ball_palettes = []
    for color in COLORS:
        sheet = fetch_image(f"ball-{color}.png")
        if sheet.width != 55 or sheet.height < 400:
            raise ValueError(f"Unexpected ball-{color}.png size: {sheet.size}")
        balls.append(sheet)
        ball_palettes.append(ball_palette(sheet))

    tile_data = bytearray()

    # 0..8: empty cell.
    tile_data.extend(image_tiles(indexed_cell))
    assert len(tile_data) == TILES_PER_CELL * TILE_BYTES

    # 9..449: seven colors x seven sizes.
    for color_index, sheet in enumerate(balls):
        colors = ball_palettes[color_index]
        for frame in range(FRAMES):
            ball = crop_frame(sheet, frame).resize(
                (BALL_SIZE, BALL_SIZE), Image.LANCZOS)
            composed = compose_ball_cell(normal_cell, ball, colors, NORMAL_RAMP)
            tile_data.extend(image_tiles(composed))

    assert len(tile_data) == (TILE_BALL_BASE + len(COLORS) *
                              TILE_COLOR_STRIDE) * TILE_BYTES

    # 450..475: letters A-Z.
    for ch in "ABCDEFGHIJKLMNOPQRSTUVWXYZ":
        tile_data.extend(glyph_tile(ch, 1, 1, 4))

    # 476..478: punctuation.
    for ch in PUNCTUATION:
        tile_data.extend(glyph_tile(ch, 1, 1, 4))

    # 479..488: score digits, scaled 2x.
    for ch in "0123456789":
        tile_data.extend(glyph_tile(ch, 2, 3, 1))

    # 489..495: preview mini-balls.
    for color_index, sheet in enumerate(balls):
        tile_data.extend(preview_tile(crop_frame(sheet, 0),
                                      ball_palettes[color_index]))

    # 496..499: solid background, panel, button and black tiles.
    tile_data.extend(solid_tile(0))
    tile_data.extend(solid_tile(6))
    tile_data.extend(solid_tile(3))
    tile_data.extend(solid_tile(7))

    # 500..503: cursor reticle corners, drawn as sprites over the board.
    for corner in range(4):
        tile_data.extend(cursor_tile(corner))

    # 504..510: large title letters, scaled 2x inside 16x16 tiles.
    for ch in TITLE_GLYPHS:
        tile_data.extend(glyph_tile(ch, 2, 3, 1))

    # 511: reserve, filled with the background color index.
    while len(tile_data) < TOTAL_TILES * TILE_BYTES:
        tile_data.extend(solid_tile(0))

    if len(tile_data) != TOTAL_TILES * TILE_BYTES:
        raise AssertionError(f"Unexpected tileset size: {len(tile_data)}")

    # Palette groups.
    normal = [(v, v, v) for v in NORMAL_RAMP]
    selected = [(v, v, v) for v in SELECTED_RAMP]
    groups = []
    groups.append(normal + [BLACK] * 8 + [WHITE])
    groups.append(selected + [BLACK] * 8 + [WHITE])
    for colors in ball_palettes:
        groups.append(normal + colors)
    for colors in ball_palettes:
        groups.append(selected + colors)
    assert len(groups) == 16
    assert all(len(g) == 16 for g in groups)

    palette_words = []
    for group in groups:
        palette_words.extend(rgb565(color) for color in group)

    header = f"""#pragma once
#include <stdint.h>

#define ZEAL_ASSET_TILE_COUNT {TOTAL_TILES}
#define ZEAL_ASSET_TILE_BYTES {TILE_BYTES}
#define ZEAL_ASSET_FILE_BYTES {TOTAL_TILES * TILE_BYTES}
#define ZEAL_ASSET_FRAME_COUNT {FRAMES}
#define ZEAL_ASSET_BALL_COLORS {len(COLORS)}

#define TILE_EMPTY {TILE_EMPTY}
#define TILE_BALL_BASE {TILE_BALL_BASE}
#define TILE_COLOR_STRIDE {TILE_COLOR_STRIDE}
#define TILE_SIZE_STRIDE {TILE_SIZE_STRIDE}
#define TILE_FONT_LETTERS {TILE_FONT_LETTERS}
#define TILE_FONT_PUNCT {TILE_FONT_PUNCT}
#define TILE_FONT_DIGITS {TILE_FONT_DIGITS}
#define TILE_PREVIEW {TILE_PREVIEW}
#define TILE_BACKGROUND {TILE_BACKGROUND}
#define TILE_PANEL {TILE_PANEL}
#define TILE_BUTTON {TILE_BUTTON}
#define TILE_BLACK {TILE_BLACK}
#define TILE_CURSOR {TILE_CURSOR}
#define TILE_TITLE {TILE_TITLE}
#define TILE_RESERVE {TILE_RESERVE}
#define ZEAL_TITLE_GLYPHS "{TITLE_GLYPHS}"

#define PALETTE_GROUP_EMPTY 0
#define PALETTE_GROUP_EMPTY_SELECTED 1
#define PALETTE_GROUP_BALL 2
#define PALETTE_GROUP_BALL_SELECTED 9

extern const uint16_t zeal_palette[256];

"""
    source = '#include "assets_generated.h"\n\n'
    source += "const uint16_t zeal_palette[256] = {\n"
    for start in range(0, 256, 8):
        chunk = palette_words[start:start + 8]
        source += "    " + ", ".join(f"0x{x:04X}" for x in chunk) + ",\n"
    source += "};\n"

    (ROOT / "src/assets_generated.h").write_text(header, encoding="utf-8")
    (ROOT / "src/assets_generated.c").write_text(source, encoding="utf-8")
    (ROOT / "assets/zeal-tiles.dat").write_bytes(tile_data)

    # Human-readable atlas preview.
    atlas = Image.new("RGB", (32 * TILE_SIZE, 16 * TILE_SIZE), (20, 20, 24))
    flat_palette = []
    for group in groups:
        for color in group:
            flat_palette.extend(color)
    for index in range(TOTAL_TILES):
        offset = index * TILE_BYTES
        tile = Image.new("P", (TILE_SIZE, TILE_SIZE))
        tile.putpalette(flat_palette)
        pixels = tile.load()
        for y in range(TILE_SIZE):
            for x in range(0, TILE_SIZE, 2):
                byte = tile_data[offset + y * 8 + x // 2]
                group = (index // 450) if index < 450 else 0
                # approximation: use group 0 for empty/fonts and the
                # first matching ball group for ball tiles
                if TILE_BALL_BASE <= index < TILE_BALL_BASE + 441:
                    group = 2 + ((index - TILE_BALL_BASE) // TILE_COLOR_STRIDE)
                pixels[x, y] = group * 16 + (byte >> 4)
                pixels[x + 1, y] = group * 16 + (byte & 15)
        x = (index % 32) * TILE_SIZE
        y = (index // 32) * TILE_SIZE
        atlas.paste(tile.convert("RGB"), (x, y))
    atlas.save(ROOT / "assets/atlas_preview.png")

    print(f"Tiles: {TOTAL_TILES}, bytes: {len(tile_data)}")
    print(f"Palette groups: {len(groups)}")
    print(f"Tileset: {ROOT / 'assets/zeal-tiles.dat'}")
    print(f"Preview: {ROOT / 'assets/atlas_preview.png'}")


if __name__ == "__main__":
    main()
