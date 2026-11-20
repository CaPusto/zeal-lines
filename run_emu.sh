#!/bin/bash
#
# Run zeal-lines in the Zeal Native Emulator.
#
# Build first with:
#     ~/zeal-dev-environment/zde cmake
#
set -e

PROJECT_BIN="${ZEAL_LINES_BIN:-$HOME/zeal-projects/zeal-lines/bin}"
EMU_DIR="${ZEAL_NATIVE_DIR:-$HOME/zeal-native-emulator}"
EMU="$EMU_DIR/zeal-native"
ROM="${ZEAL_ROM:-$EMU_DIR/zos/os_with_romdisk.img}"

if [ ! -x "$EMU" ]; then
    echo "error: zeal-native not found at $EMU" >&2
    echo "set ZEAL_NATIVE_DIR to the emulator directory if it is elsewhere" >&2
    exit 1
fi

if [ ! -f "$ROM" ]; then
    echo "error: Zeal OS ROM not found at $ROM" >&2
    echo "set ZEAL_ROM to os_with_romdisk.img if it is elsewhere" >&2
    exit 1
fi

if [ ! -f "$PROJECT_BIN/zeal-lines.bin" ]; then
    echo "error: $PROJECT_BIN/zeal-lines.bin not found" >&2
    echo "build the project with: ~/zeal-dev-environment/zde cmake" >&2
    exit 1
fi

# HostFS is exposed as H: by Zeal 8-bit OS. The game reads
# H:/zeal-tiles.dat first, so mount the bin directory.
exec "$EMU" \
    --rom "$ROM" \
    --hostfs "$PROJECT_BIN" \
    --uprog "$PROJECT_BIN/zeal-lines.bin" \
    "$@"
