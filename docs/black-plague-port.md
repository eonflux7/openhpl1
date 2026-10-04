# Black Plague and Requiem

The same executable plays all three games. Which one is picked at startup:
`OVERTURE_GAME=overture|blackplague|requiem`, or without it by the data in the
working directory (Black Plague's folder has `maps/level01_cells.dae`; Requiem
is installed in the same folder and has to be asked for).

    ./run.sh blackplague
    ./run.sh requiem

Settings, saves and the log are kept apart: `~/.frictionalgames/Penumbra/`
`BlackPlagueRemade/` and `RequiemRemade/`.

What is done and what is left: [PORTING-ROADMAP.md](../PORTING-ROADMAP.md).

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

The engine needed nothing. The binaries export their symbols, and all but a
handful of template instances of the engine's functions have the same size in
Overture's and Black Plague's.

## The games' folders are left alone

Mesh caches (`.collcach`) and AI node files that have to be rebuilt are
written to `cache/` in the game's personal folder and looked for there first;
the ones that came with the game are only read. (Before this, a run rewrote
87 cache files in Black Plague's `core/cache/`.)

## Finding out what the binaries do

`blackplague.bin` and `requiem.bin` export every class and method with full
signatures (`nm -D --defined-only ... | c++filt`). `tools/porting/script_sigs.py`
reads the script functions' argument types out of them, and
`tools/porting/dis.py bp 'cGameLever::Update'` prints a function's
disassembly with the strings and floats it refers to (32-bit x86; member
offsets are those of the 32-bit build). `tools/porting/headless.sh
blackplague` runs a game headless and silent in a scratch home.

`OVERTURE_SCRIPT="4:MoveEntityToArea(\"water1\",\"waterup1\",1.4,0.4,0.2,\"\");|9:..."`
runs script commands that many seconds after the start, with `OVERTURE_MAP`
to start in a map. `Print("...")` and the collision checks in an `if` are
enough to see in the log whether a function did its work.

`... dis.py bp 'cGameEnemyState_Infected_Hunt::OnUpdate' | python3
tools/porting/short.py` gives a view that is quicker to read: labels only
where something jumps to, no unwinding code, no string reference counting.
Many of the later games' classes are an Overture class with changes
(the Infected is the Dog): comparing the sizes of the functions in the two
symbol tables shows which ones to read at all.

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

The script functions' own code is not at their `_Generic` symbol but, without
a name, at the end of the symbol before it: follow the `call` in the wrapper.

Things that helped: the vtable offsets of `iPhysicsBody` (0x38
SetLinearVelocity, 0x70 GetMass, 0x84 AddTorque, 0x90 SetEnabled) and of the
hinge (0x3c GetAngle, 0x40/0x44 SetMax/MinAngle, 0x48/0x4c GetMax/MinAngle);
objdump prints `fsubp`/`fsubrp` and `fdivp`/`fdivrp` swapped in AT&T syntax. Comparing the calls a
function makes in Overture's binary and in Black Plague's (`objdump -d`) shows
what a changed function gained: that is how the spider hatching from its egg
was found to need `OnWorldLoad` after an object breaks.

Our AngelScript is stricter than the one of 2008: Black Plague's scripts give
`true` and `false` where a number is taken, so `SetLocalVar`, `SetGlobalVar`,
their `Create` forms and `SetBodyProperty` have bool overloads.

`tools/porting/menu_widgets.py` lists what `cMainMenu::CreateWidgets` builds:
for each widget its class, the strings and floats before it, and the menu
state it is added to (from the `0x6c + 8 * state` list it is hooked into).
That gave the later games' menu pages and their state numbers. `dis.py`
reads wide strings first (`L"EFX:"`, which as a narrow string would read
"E"). Static tables such as the button handler's default keys are filled by
an initializer the disassembly files under a neighbouring function's name
(`cButtonHandler::OnPostSceneDraw`): look for stores to fixed `0x86b....`
addresses.

Clicking in the menu with injected input (`build/porting/step3/input.py`):
the pointer now ranges over the whole wide window, so give `MENU_XOFF=0`
and window pixels. At 1280 × 720 the 800 × 600 menu is centred: pixel x =
160 + 1.2 × menu x, pixel y = 1.2 × menu y (Black Plague's main buttons at
x 424; the first at y 298, then every 61).
