# Black Plague and Requiem

The same executable plays all three games. Which one is picked at startup:
`OVERTURE_GAME=overture|blackplague|requiem`, or without it by the data in the
working directory (Black Plague's folder has `maps/level01_cells.dae`; Requiem
is installed in the same folder and has to be asked for).

    ./run.sh blackplague
    ./run.sh requiem

Settings, saves and the log are kept apart: `~/.frictionalgames/Penumbra/`
`BlackPlagueRemade/` and `RequiemRemade/`.

What is done and what is left:
[the shared roadmap](../ROADMAP.md#black-plague-and-requiem).

## How the missing code is stood in for

- **Script functions** (`game/core/GameScriptsNotPorted.cpp`): functions with
  the names and argument types of the games' binaries, which only count
  their calls. The log lists at exit which were called. A function moves to
  `GameScripts.cpp` when it is written.
- **More arguments in Black Plague** (`ChangeMap`, `SetupLink`,
  `SetupLinkLoadText`, `SetupDamageArea`) are overloads next to Overture's.
- **Enemies** of a kind that is not known are left out of the map, with a
  warning in the log.

What Black Plague changed in classes Overture has too (the enemy base class,
the player's forced look) is behind `cInit::IsOverture()` wherever Overture
would behave differently.

The initial bootstrap needed no engine additions: Black Plague and Requiem run
on essentially the same engine as Overture, and many of their classes are an
Overture class with changes (the Infected is the Dog). Subsequent porting work
also fixed shared physics, audio and other engine behaviour; see CHANGES.

## The games' folders are left alone

Mesh caches (`.collcach`) and AI node files that have to be rebuilt are
written to `cache/` in the game's personal folder and looked for there first;
the ones that came with the game are only read. (Before this, a run rewrote
87 cache files in Black Plague's `core/cache/`.)

## Testing the later games

Examples below that mention `build/porting/` experiments refer to historical
scratch scripts and outputs, which are not shipped and may no longer exist.

`tools/porting/headless.sh blackplague` runs a game headless and silent in a
scratch home.

`OVERTURE_SCRIPT="4:MoveEntityToArea(\"water1\",\"waterup1\",1.4,0.4,0.2,\"\");|9:..."`
runs script commands that many seconds after the start, with `OVERTURE_MAP`
to start in a map. `Print("...")` and the collision checks in an `if` are
enough to see in the log whether a function did its work.

`OVERTURE_ENEMY_LOG=1` writes every enemy's changes of state to the log, and
once a second where it is, whether it moves and sees the player, and its
animation. With `OVERTURE_MAP=level07_residental_corridors:link02` and
`OVERTURE_SCRIPT='5:SetGameEntityActive("inf01",true);|8:ShowEnemyPlayer("inf01");'`
that shows a whole hunt and attack. Start at a link the level is entered
by: from `link01` of that map the player lies in the level's opening scene.

In `OVERTURE_SCRIPT` the command
`save` writes an auto save and `load` loads the newest one, e.g.
`'12:save|15:load'`, for trying save games of the new entities.

`OVERTURE_PHYSLOG=1` shows bodies that never come to rest (`awake` lines);
`build/porting/limits/ab.sh GAME MAP:LINK` compared old and new this way when
the joint limits were changed (through an `OVERTURE_OLD_LIMITS` switch since
removed), and `check.sh` there tries the tower lever
and Requiem level 2's doors.

Some behaviour only shows up as a side effect: the spider hatching from its
egg needs `OnWorldLoad` after an object breaks.

Our AngelScript is stricter than the one of 2008: Black Plague's scripts give
`true` and `false` where a number is taken, so `SetLocalVar`, `SetGlobalVar`,
their `Create` forms and `SetBodyProperty` have bool overloads.

Clicking in the menu with injected input (`build/porting/step3/input.py`):
the pointer now ranges over the whole wide window, so give `MENU_XOFF=0`
and window pixels. At 1280 × 720 the 800 × 600 menu is centred: pixel x =
160 + 1.2 × menu x, pixel y = 1.2 × menu y (Black Plague's main buttons at
x 424; the first at y 298, then every 61).

## Hunting bugs in bulk

`OVERTURE_SMOKE=all OVERTURE_CHECKS=all tools/porting/headless.sh GAME 2400`
loads every map and runs the checks of `game/core/MapChecks.cpp` on each,
writing `CHECK <kind> <map>: ...` lines to the log (leads, not failures):

- `settle`: bodies that move, fall more than 2 m, keep shaking or go NaN
  in five seconds with nobody touching them, and joints that leave where
  the map put them. Scripted motion shows up too (Requiem level 4's
  generator, level 6's chain box): check the level script before
  calling one a bug.
- `pick`: things the player can use that the pick ray finds behind
  something else from every side. `pick` lines are the ones where the
  other body is less than 2 cm in front (overlap, ties); `pick-behind` lines
  are usually items in closed cabinets and can be skipped.
- `areas`: areas with a player collide callback that the game's collision
  test does not find the player in, standing in their middle.
- `joints`: doors, drawers, levers and wheels that move under 10° (5 cm)
  when pushed both ways with 15 m/s² times their mass. Locked doors are
  left out (their limits are shut). Doors the maps block with a prop show
  up too (Requiem level 2's `armoury05` behind a soda machine, level 8's
  `gdoor` behind a box).
- `links`: start positions the player falls from, is pushed off or ends
  up in the air at. Unused links of the maps show up here too.

- `calls` (not in `all`): every function of the level script is called
  once, one a frame, with empty strings and zeros, after the other checks;
  a call that changes the map is followed by a change back. It finds
  crashes and script exceptions in the game's script functions; the many
  "couldn't find" warnings it causes are expected. Load screens it opens
  are closed at once.

Give a subset as `OVERTURE_CHECKS=settle,joints`, and maps as
`OVERTURE_SMOKE=level07,level09`. Two runs at once need their own home:
`HEADLESS_HOME=<dir> HEADLESS_TAG=<name>` for the second.
`OVERTURE_CONTACT_SKIN=<metres>` overrides the contact skin, for comparing
(`build/porting/skin/crate1.sh` pushes level 4's crate).

Warnings and errors given while a script runs end with the script function
and line (`[script: void OnUpdate(), line 3101]`), and a script line the game
runs that does not compile (a callback that does not exist) or throws is
logged as `Script '...': couldn't run` / `exception`.

`tools/porting/check_scripts.py [game ...]` checks the level scripts against
the functions the game registers: callbacks that are not defined (a timer,
collide or use callback naming a missing function), functions defined twice,
calls with an argument count no overload takes. It reads the registered
functions from `build/porting/api/<game>.txt`, written by running the game
once with `OVERTURE_SCRIPT_API=<that file> OVERTURE_SMOKE=none`. On
2026-10-06 its 8 findings were all mistakes in the shipped scripts
(functions commented out), the same in the original.
