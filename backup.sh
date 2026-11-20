#!/usr/bin/env bash
#
# Create a timestamped backup of zeal-lines in ~/backups.
#
# Sources, assets and configuration files are included.
# Rebuildable artifacts (build/, bin/, generated binaries, virtualenvs) and
# runtime data (saves, high scores) are excluded.
#
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_NAME="$(basename "${PROJECT_DIR}")"
BACKUP_DIR="${HOME}/backups"
TIMESTAMP="$(date +%Y%m%d-%H%M%S)"
ARCHIVE="${BACKUP_DIR}/${PROJECT_NAME}-${TIMESTAMP}.tar.gz"

if [[ ! -f "${PROJECT_DIR}/CMakeLists.txt" ]]; then
    echo "error: ${PROJECT_DIR} does not look like the zeal-lines project root" >&2
    exit 1
fi

mkdir -p "${BACKUP_DIR}"

tar \
    --exclude='./build' \
    --exclude='./bin' \
    --exclude='./tests/build' \
    --exclude='./tmp' \
    --exclude='./.git' \
    --exclude='./.venv-assets' \
    --exclude='__pycache__' \
    --exclude='*.pyc' \
    --exclude='./assets/atlas_preview.png' \
    --exclude='*.sav' \
    --exclude='zeal-scores.dat' \
    -czf "${ARCHIVE}" \
    -C "${PROJECT_DIR}" .

echo "Backup created: ${ARCHIVE}"
echo "Size: $(du -h "${ARCHIVE}" | cut -f1)"
