#!/bin/sh
# WSL: mirrors this source tree to a folder on a Windows drive, where
# platform\windows\build.cmd builds it with MSVC. Building from a Windows
# path is much faster than from \\wsl.localhost, and avoids UNC paths.
# Usage: platform/windows/sync.sh [/mnt/e/dev/penumbra/openhpl1]
set -e
SRC="$(cd "$(dirname "$0")/../.." && pwd)"
DEST="${1:-${OVERTURE_WINDOWS_DIR:-/mnt/e/dev/penumbra/openhpl1}}"
mkdir -p "$DEST"
rsync -rlt --delete --info=stats1 \
	--exclude='/build/' --exclude='/build-*/' --exclude='/.cache/' --exclude='/.git/' \
	--exclude='__pycache__/' --exclude='/vcpkg_installed/' \
	--filter='protect /build-windows/' \
	"$SRC/" "$DEST/"
echo "Synced to $DEST"
