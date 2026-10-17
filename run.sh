#!/bin/sh
# Runs OpenHPL1 against the game data of Penumbra: Overture, or of
# Black Plague or Requiem (which are not fully ported):
#   ./run.sh                  Overture
#   ./run.sh blackplague      Black Plague
#   ./run.sh requiem          Requiem (installed in Black Plague's folder)
# Settings, saves and hpl.log: ~/.frictionalgames/Penumbra/OvertureRemade/
# (BlackPlagueRemade/, RequiemRemade/)
# Options > Change Game restarts the executable as another of the three; each
# game's folder is remembered in ~/.frictionalgames/Penumbra/games.cfg when it
# runs (Steam's default folders are found without it).
HERE="$(cd "$(dirname "$0")" && pwd)"
STEAM="$HOME/.local/share/Steam/steamapps/common"
case "$1" in
	overture|blackplague|requiem) OVERTURE_GAME="$1"; shift ;;
esac
case "$OVERTURE_GAME" in
	blackplague|requiem) DEFAULT_DIR="$STEAM/Penumbra Black Plague" ;;
	*) DEFAULT_DIR="$STEAM/Penumbra Overture" ;;
esac
export OVERTURE_GAME
GAME_DIR="${OVERTURE_GAME_DIR:-$DEFAULT_DIR}"
cd "$GAME_DIR" || { echo "Game data not found: $GAME_DIR (set OVERTURE_GAME_DIR)"; exit 1; }
exec "$HERE/build/openhpl1" "$@"
