#!/bin/sh
# Runs a game headless and silent in a scratch HOME, for tests:
#   tools/porting/headless.sh blackplague|requiem|overture [seconds]
# Environment (OVERTURE_MAP, OVERTURE_SMOKE, OVERTURE_DUMP_FRAMES, ...) is passed on.
# The log ends up in build/porting/home/.frictionalgames/Penumbra/<game>Remade/hpl.log
# HEADLESS_HOME=<dir> HEADLESS_TAG=<name> run a second one beside the first.
HERE="$(cd "$(dirname "$0")/../.." && pwd)"
GAME="${1:-blackplague}"
SECONDS_MAX="${2:-60}"
STEAM="$HOME/.local/share/Steam/steamapps/common"
case "$GAME" in
	blackplague) DIR="$STEAM/Penumbra Black Plague"; PERSONAL=BlackPlagueRemade; MAP='GlobalScript="global_script.hps" File="level01_cells.dae"'; LANG_FILE=English.lang ;;
	requiem) DIR="$STEAM/Penumbra Black Plague"; PERSONAL=RequiemRemade; MAP='GlobalScript="requiem_global_script.hps" File="level00.dae"'; LANG_FILE=English_exp.lang ;;
	*) DIR="$STEAM/Penumbra Overture"; PERSONAL=OvertureRemade; MAP=''; LANG_FILE=English.lang ;;
esac
SCRATCH="${HEADLESS_HOME:-$HERE/build/porting/home}"
mkdir -p "$SCRATCH/.frictionalgames/Penumbra/$PERSONAL"
SETTINGS="$SCRATCH/.frictionalgames/Penumbra/$PERSONAL/settings.cfg"
if [ ! -f "$SETTINGS" ]; then
	# The user's own graphics settings, in a 1280x720 window
	sed -E -e 's/(<Screen[^>]*Width=")-?[0-9]+/\11280/' -e 's/(<Screen[^>]*Height=")-?[0-9]+/\1720/' \
		-e 's/(<Screen[^>]*FullScreen=")[a-z]+/\1false/' -e 's/ShowPreMenu="true"/ShowPreMenu="false"/' \
		-e "s/LanguageFile=\"[^\"]*\"/LanguageFile=\"$LANG_FILE\"/" \
		"$HOME/.frictionalgames/Penumbra/OvertureRemade/settings.cfg" > "$SETTINGS"
	[ -n "$MAP" ] && sed -i -E "s/<Map [^>]*>/<Map $MAP StartPos=\"link01\" \/>/" "$SETTINGS"
fi
cat > "$HERE/build/porting/launch-$GAME$HEADLESS_TAG.sh" <<LAUNCH
#!/bin/sh
export HOME="$SCRATCH" ALSOFT_DRIVERS=null OVERTURE_GAME=$GAME
cd "$DIR"
exec "$HERE/build/openhpl1"
LAUNCH
chmod +x "$HERE/build/porting/launch-$GAME$HEADLESS_TAG.sh"
timeout -k 3 "$SECONDS_MAX" gamescope --backend headless -W 1280 -H 720 -w 1280 -h 720 -- "$HERE/build/porting/launch-$GAME$HEADLESS_TAG.sh" > "$HERE/build/porting/last-run-$GAME$HEADLESS_TAG.out" 2>&1
echo "exit $?  log: $SCRATCH/.frictionalgames/Penumbra/$PERSONAL/hpl.log"
