#!/usr/bin/env python3
"""
Prepare the original L2 graphics for Zeal-Lines (VideoBoard mode 6, 640x480, 4bpp).

Requires: Python 3 and Pillow (`python3 -m pip install --user Pillow`).

Run from the zeal-lines repository root:
    python3 tools/prepare_assets.py

Outputs:
    src/assets_generated.h
    src/assets_generated.c

The generated tileset contains 100 32x32 cell variants, each made of four
16x16 tiles: normal/selected empty cells plus seven animation frames for
each of the seven ball colors in normal/selected cells.
"""
from pathlib import Path
from urllib.request import urlopen

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
APP = "https://raw.githubusercontent.com/CaPusto/L2/main/src/app/"
COLORS = [
    "yellow", "green", "red", "blue", "violet", "aqua", "pink"
]
BALL_SIZE = 32
BALL_DIAMETER = 26
TILE_SIZE = 16
TILE_BYTES = 128
FRAMES = 7
VARIANTS = 2 + len(COLORS) * 2 * FRAMES


def fetch_image(name):
    url = APP + name
    with urlopen(url, timeout=30) as response:
        data = response.read()
    from io import BytesIO
    return Image.open(BytesIO(data)).convert("RGBA")


def crop_frame(sheet, frame):
    # The original CSS advances the background by 60 px per animation state.
    # Pad the last frame if the source sprite sheet ends before y=420.
    y = frame * 60
    frame_image = Image.new("RGBA", (55, 55), (0, 0, 0, 0))
    part = sheet.crop((0, y, 55, min(y + 55, sheet.height)))
    frame_image.alpha_composite(part, (0, 0))
    return frame_image


def rgb565(rgb):
    r, g, b = rgb[:3]
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def palette_for(images):
    sheet = Image.new("RGB", (BALL_SIZE, BALL_SIZE * len(images)))
    for i, image in enumerate(images):
        sheet.paste(image.convert("RGB"), (0, i * BALL_SIZE))
    indexed = sheet.quantize(colors=16, method=Image.Quantize.MEDIANCUT,
                             dither=Image.Dither.NONE)
    raw = indexed.getpalette()[:48]
    raw += [0] * (48 - len(raw))
    return [tuple(raw[i:i + 3]) for i in range(0, 48, 3)]


def indexed_image(image, palette):
    pal = []
    for color in palette:
        pal.extend(color)
    image = image.convert("RGB").quantize(
        colors=16, palette=make_palette_image(pal),
        dither=Image.Dither.NONE
    )
    return image


def make_palette_image(rgb_values):
    p = Image.new("P", (1, 1))
    p.putpalette(rgb_values + [0] * (768 - len(rgb_values)))
    return p


def image_tiles(indexed):
    pixels = indexed.load()
    result = bytearray()
    for ty in range(2):
        for tx in range(2):
            for y in range(TILE_SIZE):
                for x in range(0, TILE_SIZE, 2):
                    px = pixels[tx * TILE_SIZE + x, ty * TILE_SIZE + y] & 15
                    py = pixels[tx * TILE_SIZE + x + 1, ty * TILE_SIZE + y] & 15
                    result.append((px << 4) | py)
    assert len(result) == TILE_BYTES * 4
    return result


def c_array(name, ctype, values, per_line=16):
    lines = [f"const {ctype} {name}[{len(values)}] = {{"]
    for start in range(0, len(values), per_line):
        chunk = values[start:start + per_line]
        if ctype == "uint16_t":
            rendered = ", ".join(f"0x{x:04X}" for x in chunk)
        else:
            rendered = ", ".join(f"0x{x:02X}" for x in chunk)
        lines.append("    " + rendered + ",")
    lines.append("};")
    return "\n".join(lines)


def main():
    ROOT.joinpath("src").mkdir(exist_ok=True)
    cell_sheet = fetch_image("cell-bgr.png")
    if cell_sheet.size != (69, 138):
        raise ValueError(f"Unexpected cell-bgr.png size: {cell_sheet.size}")
    normal_cell = cell_sheet.crop((0, 0, 69, 69)).resize(
        (32, 32), Image.Resampling.LANCZOS
    ).convert("RGB")
    selected_cell = cell_sheet.crop((0, 69, 69, 138)).resize(
        (32, 32), Image.Resampling.LANCZOS
    ).convert("RGB")

    # Variant 0: normal empty cell. Variant 1: selected empty cell.
    images_by_group = {
        0: [normal_cell],
        1: [selected_cell],
    }
    variants = [
        (normal_cell, 0),
        (selected_cell, 1),
    ]

    for color_index, color in enumerate(COLORS):
        sheet = fetch_image(f"ball-{color}.png")
        if sheet.width != 55 or sheet.height < 400:
            raise ValueError(
                f"Unexpected ball-{color}.png size: {sheet.size}"
            )
        for selected in (False, True):
            group = 2 + color_index + (7 if selected else 0)
            base = selected_cell if selected else normal_cell
            frames = []
            for frame in range(FRAMES):
                ball = crop_frame(sheet, frame).resize(
                    (BALL_DIAMETER, BALL_DIAMETER), Image.Resampling.LANCZOS
                )
                composed = base.convert("RGBA")
                composed.alpha_composite(ball, (3, 3))
                frames.append(composed.convert("RGB"))
                variants.append((composed.convert("RGB"), group))
            images_by_group[group] = frames

    # Build one 16-color palette per group so each ball keeps its own shading.
    palettes = [[(0, 0, 0)] * 16 for _ in range(16)]
    palettes[0] = palette_for([normal_cell])
    palettes[1] = palette_for([selected_cell])
    for group, images in images_by_group.items():
        if group >= 2:
            palettes[group] = palette_for(images)

    palette_words = []
    for group in range(16):
        palette_words.extend(rgb565(c) for c in palettes[group])

    tile_data = bytearray()
    for image, group in variants:
        indexed = indexed_image(image, palettes[group])
        tile_data.extend(image_tiles(indexed))

    expected_tiles = VARIANTS * 4
    if len(tile_data) != expected_tiles * TILE_BYTES:
        raise AssertionError("Generated tileset has an unexpected size")

    header = """#pragma once
#include <stdint.h>

#define ZEAL_ASSET_TILE_COUNT %d
#define ZEAL_ASSET_VARIANT_COUNT %d
#define ZEAL_ASSET_TILE_BYTES %d

extern const uint16_t zeal_palette[256];
extern const uint8_t zeal_tileset[%d];

""" % (expected_tiles, VARIANTS, TILE_BYTES, len(tile_data))

    source = '#include "assets_generated.h"\n\n'
    source += c_array("zeal_palette", "uint16_t", palette_words, 8)
    source += "\n\n"
    source += c_array("zeal_tileset", "uint8_t", list(tile_data), 16)
    source += "\n"

    (ROOT / "src/assets_generated.h").write_text(header, encoding="utf-8")
    (ROOT / "src/assets_generated.c").write_text(source, encoding="utf-8")
    print(f"Generated {expected_tiles} tiles ({len(tile_data)} bytes)")
    print(f"Generated 16 palette groups ({len(palette_words)} RGB565 colors)")
    print("Files: src/assets_generated.h, src/assets_generated.c")


if __name__ == "__main__":
    main()
