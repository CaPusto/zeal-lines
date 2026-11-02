#!/bin/bash

# Zeal-lines - Run in Emulator Script

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Environment setup
export PATH=/home/ed/work/tools/sdcc-4.4.0/bin:/home/ed/work/zeal-native-linux-x64:$PATH
export ZOS_PATH=/home/ed/work/sdk/Zeal-8-bit-OS
export ZVB_SDK_PATH=/home/ed/work/sdk/Zeal-VideoBoard-SDK
export ZGDK_PATH=/home/ed/work/sdk/zeal-game-dev-kit
export ZEAL_NATIVE=/home/ed/work/zeal-native-linux-x64

# Check if binary exists
if [ ! -f "$SCRIPT_DIR/zeal_lines.bin" ]; then
    echo "Error: zeal_lines.bin not found in $SCRIPT_DIR"
    exit 1
fi

# Run emulator
cd "$SCRIPT_DIR"

echo "Starting Zeal 8-bit Emulator with zeal-lines..."
echo "Binary: $SCRIPT_DIR/zeal_lines.bin"
echo ""

zeal-native --rom $ZEAL_NATIVE/zos/os_with_romdisk.img \
    -u ./zeal_lines.bin

exit $?
