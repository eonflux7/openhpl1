#!/bin/sh
# Runs Overture Remade against the Penumbra: Overture game data.
# Settings, saves and hpl.log: ~/.frictionalgames/Penumbra/OvertureRemade/
HERE="$(cd "$(dirname "$0")" && pwd)"
GAME_DIR="${OVERTURE_GAME_DIR:-$HOME/.local/share/Steam/steamapps/common/Penumbra Overture}"
cd "$GAME_DIR" || { echo "Game data not found: $GAME_DIR (set OVERTURE_GAME_DIR)"; exit 1; }
exec "$HERE/build/overture" "$@"
