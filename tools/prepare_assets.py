#!/usr/bin/env python3
"""
Prepare L2 graphics for Zeal-Lines.
VideoBoard: 640x480, 4bpp.

Each cell is 48x48 pixels (3x3 tiles).
The tileset uses 144 tile slots per animation frame.
Seven animation frames are stored sequentially in zeal_tiles.bin.
"""

from pathlib import Path
from urllib.request import urlopen
from io import BytesIO

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
APP = "https://raw.githubusercontent.com/CaPusto/L2/main/src/app/"

COLORS = [
    "yellow", "green", "red", "blue",
    "violet", "aqua", "pink"
]

CELL_SIZE = 48
BALL_DIAMETER = 26

TILE_SIZE = 16
TILE_BYTES = 128
TILES_PER_CELL = 9

FRAMES = 7
VARIANTS = 2 + len(COLORS) * 2

TILES_PER_FRAME = VARIANTS * TILES_PER_CELL
FRAME_BYTES = TILES_PER_FRAME * TILE_BYTES


def fetch_image(name):
    url = APP + name

    with urlopen(url, timeout=30) as response:
        data = response.read()

    return Image.open(BytesIO(data)).convert("RGBA")


def crop_frame(sheet, frame):
    # Original L2 sprite animation advances by 60 pixels.
    y = frame * 60

    frame_image = Image.new(
        "RGBA", (55, 55), (0, 0, 0, 0)
    )

    part = sheet.crop(
        (0, y, 55, min(y + 55, sheet.height))
    )

    frame_image.alpha_composite(part, (0, 0))

    return frame_image


def rgb565(rgb):
    r, g, b = rgb[:3]

    return (
        ((r >> 3) << 11) |
        ((g >> 2) << 5) |
        (b >> 3)
    )


def make_palette_image(rgb_values):
    p = Image.new("P", (1, 1))

    p.putpalette(
        rgb_values + [0] * (768 - len(rgb_values))
    )

    return p


def palette_for(images):
    sheet = Image.new(
        "RGB",
        (CELL_SIZE, CELL_SIZE * len(images))
    )

    for i, image in enumerate(images):
        sheet.paste(
            image.convert("RGB"),
            (0, i * CELL_SIZE)
        )

    indexed = sheet.quantize(
        colors=16,
        method=Image.Quantize.MEDIANCUT,
        dither=Image.Dither.NONE
    )

    raw = indexed.getpalette()[:48]
    raw += [0] * (48 - len(raw))

    return [
        tuple(raw[i:i + 3])
        for i in range(0, 48, 3)
    ]


def indexed_image(image, palette):
    pal = []

    for color in palette:
        pal.extend(color)

    return image.convert("RGB").quantize(
        colors=16,
        palette=make_palette_image(pal),
        dither=Image.Dither.NONE
    )


def image_tiles(indexed):
    pixels = indexed.load()
    result = bytearray()

    for ty in range(3):
        for tx in range(3):

            for y in range(TILE_SIZE):
                for x in range(0, TILE_SIZE, 2):

                    px = pixels[
                        tx * TILE_SIZE + x,
                        ty * TILE_SIZE + y
                    ] & 15

                    py = pixels[
                        tx * TILE_SIZE + x + 1,
                        ty * TILE_SIZE + y
                    ] & 15

                    result.append((px << 4) | py)

    assert len(result) == TILE_BYTES * TILES_PER_CELL

    return result


def c_array(name, ctype, values, per_line=16):
    lines = [
        f"const {ctype} {name}[{len(values)}] = {{"
    ]

    for start in range(0, len(values), per_line):
        chunk = values[start:start + per_line]

        if ctype == "uint16_t":
            rendered = ", ".join(
                f"0x{x:04X}" for x in chunk
            )
        else:
            rendered = ", ".join(
                f"0x{x:02X}" for x in chunk
            )

        lines.append("    " + rendered + ",")

    lines.append("};")

    return "\n".join(lines)


def main():
    ROOT.joinpath("src").mkdir(exist_ok=True)
    (ROOT / "assets").mkdir(exist_ok=True)

    # Load and prepare the two cell backgrounds.
    cell_sheet = fetch_image("cell-bgr.png")

    if cell_sheet.size != (69, 138):
        raise ValueError(
            f"Unexpected cell-bgr.png size: {cell_sheet.size}"
        )

    normal_cell = cell_sheet.crop(
        (0, 0, 69, 69)
    ).resize(
        (CELL_SIZE, CELL_SIZE),
        Image.Resampling.LANCZOS
    ).convert("RGB")

    selected_cell = cell_sheet.crop(
        (0, 69, 69, 138)
    ).resize(
        (CELL_SIZE, CELL_SIZE),
        Image.Resampling.LANCZOS
    ).convert("RGB")

    # Store all seven frames for each color and cell state.
    frames_by_group = {}

    palettes = [[(0, 0, 0)] * 16 for _ in range(16)]

    palettes[0] = palette_for([normal_cell])
    palettes[1] = palette_for([selected_cell])

    for color_index, color in enumerate(COLORS):

        sheet = fetch_image(f"ball-{color}.png")

        if sheet.width != 55 or sheet.height < 400:
            raise ValueError(
                f"Unexpected ball-{color}.png size: {sheet.size}"
            )

        for selected in (False, True):

            group = (
                2 + color_index +
                (7 if selected else 0)
            )

            base = selected_cell if selected else normal_cell

            frames = []

            for frame in range(FRAMES):

                ball = crop_frame(
                    sheet, frame
                ).resize(
                    (BALL_DIAMETER, BALL_DIAMETER),
                    Image.Resampling.LANCZOS
                )

                composed = base.convert("RGBA")

                # Center the 26x26 ball inside the 48x48 cell.
                composed.alpha_composite(
                    ball,
                    (
                        (CELL_SIZE - BALL_DIAMETER) // 2,
                        (CELL_SIZE - BALL_DIAMETER) // 2
                    )
                )

                frames.append(composed.convert("RGB"))

            frames_by_group[group] = frames
            palettes[group] = palette_for(frames)

    # Build the seven pages.
    tile_data = bytearray()

    for frame in range(FRAMES):

        variants = [
            (normal_cell, 0),
            (selected_cell, 1),
        ]

        for color_index in range(len(COLORS)):

            normal_group = 2 + color_index
            selected_group = 9 + color_index

            variants.append((
                frames_by_group[normal_group][frame],
                normal_group
            ))

            variants.append((
                frames_by_group[selected_group][frame],
                selected_group
            ))

        if len(variants) != VARIANTS:
            raise AssertionError("Unexpected variant count")

        for image, group in variants:

            indexed = indexed_image(
                image,
                palettes[group]
            )

            tile_data.extend(image_tiles(indexed))

    expected_size = FRAME_BYTES * FRAMES

    if len(tile_data) != expected_size:
        raise AssertionError(
            f"Unexpected tileset size: {len(tile_data)}"
        )

    # Palette: 16 groups * 16 colors.
    palette_words = []

    for group in range(16):
        palette_words.extend(
            rgb565(c) for c in palettes[group]
        )

    header = """#pragma once
#include <stdint.h>

#define ZEAL_ASSET_TILE_COUNT %d
#define ZEAL_ASSET_VARIANT_COUNT %d
#define ZEAL_ASSET_TILE_BYTES %d
#define ZEAL_ASSET_FRAME_COUNT %d
#define ZEAL_ASSET_FRAME_BYTES %d

extern const uint16_t zeal_palette[256];

""" % (
        TILES_PER_FRAME,
        VARIANTS,
        TILE_BYTES,
        FRAMES,
        FRAME_BYTES
    )

    source = '#include "assets_generated.h"\n\n'

    source += c_array(
        "zeal_palette",
        "uint16_t",
        palette_words,
        8
    )

    source += "\n"

    (ROOT / "src/assets_generated.h").write_text(
        header,
        encoding="utf-8"
    )

    (ROOT / "src/assets_generated.c").write_text(
        source,
        encoding="utf-8"
    )

    tiles_path = ROOT / "assets" / "zeal_tiles.bin"
    tiles_path.write_bytes(tile_data)

    print(
        f"Generated {TILES_PER_FRAME} tile slots per frame"
    )

    print(
        f"Generated {FRAMES} animation frames"
    )

    print(
        f"Generated {len(tile_data)} bytes of tile data"
    )

    print(
        f"Generated {len(palette_words)} RGB565 palette entries"
    )

    print(
        f"Tileset: {tiles_path}"
    )


if __name__ == "__main__":
    main()