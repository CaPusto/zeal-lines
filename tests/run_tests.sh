#!/usr/bin/env bash
#
# Host-side tests for the pure game logic in src/game.c.
#
# The ZGDK random helpers are replaced by deterministic stubs, so the tests
# do not need the Zeal toolchain or the emulator.
#
set -euo pipefail

TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "${TEST_DIR}")"
BUILD_DIR="${TEST_DIR}/build"
CC="${CC:-gcc}"

mkdir -p "${BUILD_DIR}"

"${CC}" -std=c99 -Wall -Wextra \
    -I "${TEST_DIR}/stub" \
    -I "${ROOT_DIR}/src" \
    "${TEST_DIR}/test_game.c" \
    "${ROOT_DIR}/src/game.c" \
    -o "${BUILD_DIR}/test_game"

"${BUILD_DIR}/test_game"
