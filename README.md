# OpenHPL1

A modernised **HPL1 engine** and source port of **Penumbra: Overture**, with
experimental support for **Black Plague** and **Requiem**. Built from
Frictional Games' open-source HPL1 engine and Overture game code, it runs
natively on 64-bit Linux and Windows against the data from your own games.

The goal is to keep the game playing exactly like the original while bringing
the engine up to date: native widescreen, a modern renderer and a long list of
quality-of-life fixes. See the shared [roadmap](ROADMAP.md) and
[documentation index](docs/README.md).

> You need your own copy of each game you want to play. This repository
> contains no game assets.

The project was previously called Overture Remade; its repository and checkout
are now named `openhpl1`, as is the executable (`openhpl1.exe` on Windows).
Existing `OVERTURE_*` variables and per-game settings/save folders keep their
names for compatibility.

## Status

Overture is the main supported game. Black Plague and Requiem load their maps
and have substantial game code implemented, but still need playthrough testing
and have missing features. See the
[shared roadmap](ROADMAP.md#black-plague-and-requiem) and
[the porting notes](docs/black-plague-port.md) for their current limitations.

**Milestone 0 done:** the game builds and runs as a native 64-bit executable
on Linux and Windows. It boots, plays the intro and menus and loads levels at
full speed.
Full playthroughs are not tested yet, and physics behaviour still has to be
compared against the original.

Renderer cleanup is through **Milestone 6, step 13**: mesh vertices upload
interleaved into one buffer, surface geometry uses triangle indices, and
UVs and tangents have named attributes. Textures use immutable storage;
animation frames occupy layers of one texture array. The shared triangle
sprite batcher retains submission order at equal depth, including keypad
panels over inventory borders. Linked programs keep material values in
uniform blocks and draw matrices travel together. The SDL graphics backend
now delegates to separate owners for the window/context and capture, GL state
and samplers, scene/output targets, final passes, shadow targets, and streamed
sprite/triangle/debug drawing. The renderer-facing interface and frame order
stay the same. Billboards, beams and particles now use compact instance
records and shared static triangle indices; their corners expand in vertex
shaders. CPU particle simulation, facing, UVs, colours and transparent order
are retained. Particle vertex uploads use 88 bytes per live particle instead
of 176, without maximum-size emitter vertex/index arrays.

Linux build and both regression suites pass, including EGL backend checks.
The renderer suite also passes on GL 3.3 with storage extensions and GL 4.3.
For step 10, the Windows MSVC build passed; comparisons against step 9 matched
95/95 default, 95/95 motion-blur, 31/31 scaled/FSAA and 29/29 effects-lab frames.
All UI frames, effects pixel/live checks and GL debug validation passed for
that step. Details are in
[CHANGES.md](CHANGES.md#step-10-billboards-beams-and-particles).

Step 11 keeps see-through draws in explicit submission-ordered records, supports
every visible light instead of stopping at 30, removes dead per-texture/queue
interfaces, caches resolved scene depth until a real depth write, and restores
fullscreen-pass state from the state and target owners. Both CTest suites pass
in RelWithDebInfo and ASan/UBSan builds, including EGL on GL 3.3 and GL 4.3.
Fresh visual captures and Windows validation were unavailable in the
implementation environment; follow-up results are recorded below. See
the [archived renderer cleanup report](docs/archive/renderer-cleanup-plan.md).

Step 12 skips redundant linked-program/material-buffer bindings and uploads
only the interval of changed material parameters. EGL call-count checks reduce
128 unchanged submissions from 128 binds of each kind to zero; a scalar edit
uploads 16 bytes instead of the full 2,160-byte block. Frame-time effects have
not been measured. Both regression suites pass normally and with ASan/UBSan
(leak detection disabled); EGL also passes on GL 3.3 and GL 4.3.

Step 13 retains shadow depth per light under a default 128 MiB cache budget.
Unchanged maps are reused; moving casters, geometry/alpha updates and changed
light projections invalidate them. Cube faces are tracked separately as the
camera turns. F4's Shadows window shows reuse counts and lets the budget be
set to zero for comparison. See [docs/shadow-cache.md](docs/shadow-cache.md)
for the ownership and invalidation contracts. Full-game frame-time effects
remain unmeasured. Follow-up Windows validation compares pre-step-11
`c2ea1a1` with `59f1b97`: all UI screens and 29 effects frames match, but
boat-cabin and ice-cave scene differences remain at unchanged tolerances.
The current MSVC build and isolated 28-map smoke/save/reload pass; intermittent
startup access violations also occurred in baseline and current captures.
Those captured crashes were subsequently traced to EFX slot mutex lifetime
and fixed; see the [audio validation](CHANGES.md#efx-slot-startup-crashes-2026-10-19).
See the [recorded results](CHANGES.md#renderer-steps-1113-game-validation-2026-10-19)
and the
[validation checklist](ROADMAP.md#validation-to-close-first).

OpenGL 4.3 is sufficient. Earlier core contexts (3.3 or newer) need both
`GL_ARB_texture_storage` and `GL_ARB_texture_storage_multisample`; missing
storage support is reported at startup.

## What's different from the original

The short list. The full list is in [CHANGES.md](CHANGES.md).

- 64-bit, modern compilers (C++17, GCC 16 / Clang 22)
- SDL 1.2 → **SDL3**: native Wayland, raw mouse input, borderless fullscreen at
  desktop resolution, picks the largest monitor
- Nvidia Cg → **GLSL** shaders (ported by the ScummVM project); GLee/GLU →
  **glad**
- Closed 32-bit Newton 1.53 → open-source **Newton Game Dynamics 2.36**
- AngelScript 2.7 → **AngelScript 2.38**; the original game scripts run
  unchanged
- Music no longer stutters on modern Linux audio stacks (PipeWire/OpenAL Soft)
- Separate settings and save folder, so the original game is never touched

## Building

Dependencies (Fedora package names):

```
sudo dnf install gcc-c++ cmake ninja-build SDL3-devel SDL3_image-devel \
    openal-soft-devel libvorbis-devel libogg-devel mesa-libGL-devel
```

Newton, AngelScript, OALWrapper, Dear ImGui and the glad OpenGL loader are
vendored in `vendor/`.

FreeType renders the fonts. With `freetype-devel` installed the system library
is used; without it, CMake downloads FreeType 2.13.3 and builds it into the
engine. The fonts themselves are in `fonts/`.

```
cmake --preset default
cmake --build --preset default
```

This produces `build/openhpl1`, with the shaders copied to `build/shaders/`.
Other presets: `debug` (`build-debug/`) and `asan` (`build-asan/`, Debug with
AddressSanitizer and UBSan; needs `libasan libubsan`).

Run the engine checks with `ctest --test-dir build --output-on-failure`.
The audio suite uses OpenAL Soft's null driver to check EFX slot locking and
repeated device startup/teardown with and without the updater thread. It needs
no game data, display or audio output. See the [audio lifecycle notes](docs/audio.md).
With EGL headers/library installed, this also builds `renderer_regressions`,
which tests program submission, transparent ordering, shadow caching,
immutable textures, animation sampling, instanced effect geometry,
soft-particle depth fades, shader reloads, framebuffer transitions, final-pass
state restoration, render-scale/FSAA resizing and GL resource lifetime on a
surfaceless software GL context. These checks need neither game data nor
gamescope; CTest reports a skip if that GL context is unavailable.


Static analysis uses `clang-tools-extra`. `cmake --build build --target tidy`
runs the checks in `.clang-tidy` over the engine and game sources, and
`git clang-format` formats changed lines.

### Windows

Needs Visual Studio 2022 or later with the C++ workload, CMake and
[vcpkg](https://vcpkg.io) (`VCPKG_ROOT`, default `C:\vcpkg`). vcpkg builds the
dependencies listed in `vcpkg.json` on the first configure.

```
platform\windows\build.cmd
```

This produces `build-windows\openhpl1.exe` with its DLLs, shaders, fonts and
test maps. `platform\windows\run.cmd` starts it against the Steam copy
(`...\steamapps\common\Penumbra Overture\redist`; set `OVERTURE_GAME_DIR`
for another), and `platform\windows\smoke.cmd` runs the smoke test with its
own settings, in a hidden window and with OpenAL Soft's null backend, so
nothing is shown or heard. Settings, saves and `hpl.log` are in
`Documents\Penumbra Overture\OvertureRemade\`, apart from the original
game's `Episode1`. The test programs are Linux-only and not built with MSVC.

From WSL, `platform/windows/sync.sh` mirrors the source tree to a Windows
drive (default `/mnt/e/dev/penumbra/openhpl1`) to build it there.

Before changing gameplay code, run the smoke test on the ASan build:

```
cd "<game dir>" && OVERTURE_SMOKE=1 ASAN_OPTIONS=detect_leaks=0 /path/to/build-asan/openhpl1
```

Original-asset Dog behaviour and playback checks run in a silent headless
session with isolated settings, saves, maps and caches:

```
python3 tools/check_dog_foundations.py --build build-asan
```

They require gamescope and a build with `BUILD_TESTING=ON`. See
[Dog foundations](docs/dog-foundations.md) for scope, balance implications,
and the full-map check.

### Dog behaviour test map

```
OVERTURE_MAP=dogtest ./run.sh
```

F2 opens the Dog controls: select a bay and **Reset and play**, compare
Original/Improved AI, play sound lures, reveal a position or knock the Dog
down. Bays cover patrol/turning, losing sight/search, hearing, obstacle
recovery, door breaking and attack/get-up. Start directly in a bay with
`OVERTURE_MAP=dogtest:sound` (`patrol`, `hide`, `sound`, `props`, `door`,
`attack`). The build generates the arena using original game entities.

**Options → AI** also saves the Original/Improved Dog behaviour choice for
normal gameplay. Improved is the branch default; both use the original
assets and shared animation/stability fixes. See
[Dog foundations](docs/dog-foundations.md#ai-choice-and-test-arena) for details.

### Physics test map

`testmap/generate.py` builds a grey test hall (run automatically by the build,
into `build/testmap/`) with the game's doors, drawers, levers, valves,
grabbable and pushable objects, stacks, steps, ramps, a ladder and liquid,
force and stick areas. Start in it with the physics log on:

```
OVERTURE_MAP=testmap OVERTURE_PHYSLOG=1 ./run.sh
grep PHYS: ~/.frictionalgames/Penumbra/OvertureRemade/hpl.log
```

`OVERTURE_MAP=testmap:<start>` starts at one of the zones: `doors`,
`furniture`, `levers`, `grab`, `stacks`, `push`, `pool`, `steps`, `ladder`.
The log records when each body wakes and comes to rest (time, distance, peak
speed, joint angle or distance), bodies that never settle, and what the player
grabs and releases. Both variables work with the original maps too.

### Effects lab

The build also generates a small room for player feedback:

```
OVERTURE_MAP=effectslab:link01 ./run.sh
```

Its effects panel opens automatically; F2 toggles it. Pick a preview, adjust its amount, animate it or use **Tour all**. Close F2 to walk
while the preview runs. Previews cover night adaptation, low health, blood and
ice hits, panic, white flashes, save-point haze, death, underwater and combined
feedback. **Double vision only** shows the displaced outline without the red
vignette; it is also included in **Low health**. **New splash** changes the blood preview's pattern. **Stop preview**
returns to the real game effects.

The dark alcove on the left triggers real adaptation: turn the flashlight
off, crouch and stay still. The pool on the right triggers underwater feedback;
the artefact ahead is a working save point. Live buttons trigger harmless hit
animations, a flash, shake, ear ringing, scripted fear and a black fade. Health
buttons exercise the real heartbeat and movement penalties. Focus blur, grain
and boat rocking can also be tested. **Full debug panels** reveals the existing
renderer controls. From other maps, use **Player > Player effects > Load effects
lab** in F2.

Player feedback is a shader below the HUD with subtle FOV zoom:
night adaptation lifts dark detail, adds a blue cast and a faint vignette;
panic pulses the zoom and blur. Blood hits add soft splashes and droplets with
a new pattern on each hit, alongside the red vignette. Low health has a stronger
heartbeat vignette and gently pulsing vision doubling that remains visible
between heartbeats and eases away after healing; ice hits use noisy frost.
White flashes spread highlights into whiteout; save points add warm haze and
glow without the strong zoom or image trail. Underwater adds a water-colored
tint and light blur; death fades smoothly to black. The game's original
sprites, projection changes and image trails are gone (Milestone 6). The five
sliders are kept in `settings.cfg` under `Graphics` as
`PlayerAdaptationStrength`, `PlayerAdaptationTime`,
`PlayerInjuryStrength`, `PlayerPanicStrength` and `PlayerWhiteoutGlow`.

To capture and check every preview plus live adaptation, injury, fear and
underwater transitions in an isolated headless session:

```
tools/refshots.py effects /tmp/effects
```

It writes 29 PNGs and the logs, checks correct aspect and subtle/pulsing zoom,
neutral output, varied but stable blood patterns and full white/black endpoints,
and exits with 1 if a check fails. Live checks also exercise real save points,
underwater triggers, double vision between heartbeats, healing and an
interrupted save. It accepts
the same `--size`, `--set` and `--build` options as reference captures.

### Reference images

`tools/refshots.py` checks that a change leaves the rendering alone. It runs
the game in a headless gamescope session, with its own settings file and
OpenAL Soft's null audio backend, so your settings, screen and speakers are
not used:

```
tools/refshots.py capture /tmp/ref            # before the change
tools/refshots.py capture /tmp/new            # after
tools/refshots.py compare /tmp/ref /tmp/new --diff-dir /tmp/diff
```

A capture (about a minute) visits every map of the game (not the physics
test map). A map's frames are taken from
its viewpoints in `tools/refviews.txt`, which are chosen in the game: F2
opens the reference view panel, Fly moves the camera freely once the panel
is closed again (movement keys, Run for speed, Jump and Crouch for up and
down), and "Add viewpoint here" saves where the camera is and looks. A map
without viewpoints is shot from automatic places instead: each start
position, and up to six of the map's lights, looking in four directions
from each. All with the flashlight on. These frames show the 3D view only, without the HUD, fades
or menus. Then it loads the first map again and saves the 2D screens over
it (`2d-*.png`): HUD, inventory, notebook, keypad, a message, the main
menu pages, death, intro and credits. All at 1280x720 with fixed graphics
settings: shadows,
ambient occlusion, SMAA and 4x multisampling on, the noise filter off.
`--maps office,worm` limits it to
some maps (compare such a capture only with one of the same maps: frames
depend on the maps visited before), `--size` changes the resolution and
`--set Graphics.Shadows=2` any setting (2 is shadows off).

The game has one look since Milestone 6: the improved one, which the
original 2007 look was the reference for. The tag `original-look` marks the
last commit that has the original look as well, and the reference images
of it are kept in the Windows kit (`captures/original-look`). `compare` lists the frames that differ and exits with 1 if any
do. From WSL, `--windows --build <build-windows>` runs a Windows build instead
(`--build` may be a path on a Windows drive), with its window hidden; pass
the Windows game data with `--game-dir`.

Two captures of the same build are identical except for a few pixels in a
few frames, which `compare` ignores (`--tolerance`, `--max-pixels`), and
possibly for `level01_19_door_shaft` and `level01_20_base_entrance`, whose
scripted light sequences are timed partly by sounds. Their differences are
listed as "unstable" and do not count.

Step 10 passes the Linux build and both CTest suites, including real GL
instance expansion, blend/animation and soft-particle depth checks on Mesa
llvmpipe. The renderer suite also passes on GL 3.3 with storage extensions
and GL 4.3. The separate Windows kit passes its MSVC build (CTest targets
are disabled there). The indexed stage matches all 95 default step 9 frames;
final motion blur matches 95/95, Render Scale 1.5 with 4× FSAA matches 31/31,
and the effects lab matches 29/29, at unchanged tolerances. All UI frames,
effects pixel checks/live transitions and GL debug validation pass.
See [CHANGES.md](CHANGES.md) for the implementation and results.

To look at those places yourself, press F2 in the game (or start it with
`OVERTURE_REFVIEW=1`): a panel lists the maps, and for the loaded map the
places and directions of its frames, under the names the frames are saved
with. Picking one loads the map or puts you there; PgUp and PgDn step through
the map's frames. The view can be set up as in the frames (no flicker and a
player that cannot be hurt, which are on; flashlight on and no HUD, which
are off until switched), and
the lighting can be tuned from the panel: Light power, Exposure, Glow, Blend
match and the tonemapping's Shoulder and White. The bloom can be switched
off and has its own sliders (see CHANGES; Ctrl+click a slider to type a
value). A second window, Shadows, switches shadows and has the maps'
settings, a view of the shadows alone, the lights in view (to
draw one alone, or switch its shadows) and a test light to put anywhere or
carry around. A third, Player, has Fly (a free camera; switching it off puts
the player where the camera is), the switches for flashlight, flicker,
HUD and damage, and buttons that put the flashlight, the glow stick or a
flare in the inventory. A fourth, Ambient occlusion, switches it and has its
quality, radius, strength and filter, and a view of the occlusion alone.
A fifth, Lens and film, has the depth of field (with a switch to try it at
a distance of your choosing), motion blur and noise filter, and their
settings. A sixth, Anti-aliasing, is folded at the bottom right:
multisampling, the texture filter, and SMAA with its settings and a view of
the edges it finds.
While the panel is open it
has the mouse and keyboard and the view is held on the frame. F2 closes it
and gives them back, to look and walk around from that place; at a light the
camera stays at the light until "Release". The live view is not a capture:
graphics settings are your own, and the scene keeps moving.

## Running

Options → Graphics offers **High**, **Medium** and **Low** quality presets,
plus **Custom** when settings are changed by hand. First run uses High at the
desktop resolution. The renderer uses the improved look throughout.

```
./run.sh
./run.sh blackplague
./run.sh requiem
```

`run.sh` changes into the game data directory and starts the executable. By
default it looks in the Steam location
`~/.local/share/Steam/steamapps/common/Penumbra Overture`, or `Penumbra Black
Plague` for Black Plague and Requiem; set `OVERTURE_GAME_DIR` to use another
install. Requiem uses the Black Plague installation with the expansion added.

Settings, saves and the log (`hpl.log`) are stored in
`~/.frictionalgames/Penumbra/OvertureRemade/`. The original game's folder
(`.../Penumbra/Overture/`) is never read or written, so you can still play the
original side by side. Copy your old `save/` folder over if you want your
saves.

### Environment variables

| Variable | Purpose |
|---|---|
| `OVERTURE_GAME` | Select `overture`, `blackplague` or `requiem`; `run.sh` also accepts the game as its first argument |
| `OVERTURE_GAME_DIR` | Game data directory used by `run.sh` |
| `OVERTURE_PROFILE_ROOT` | Override the personal-directory root for isolated settings, saves and logs; mesh/animation caches go under `<root>/cache/` |
| `OVERTURE_DOG_REGRESSIONS=1` | Run original-asset Dog/arena checks when smoke loads `testmap` / `dogtest` (requires `BUILD_TESTING=ON`); use `tools/check_dog_foundations.py` |
| `OVERTURE_DISPLAY` | Monitor index (0-based) to open on, instead of the "Display" option; that defaults to the largest monitor |
| `OVERTURE_SHADER_DIR` | Load shaders from here instead of `<exe dir>/shaders/` |
| `OVERTURE_SHADER_RELOAD=1` | Recompile shaders when their files change while the game runs (pair with `OVERTURE_SHADER_DIR=<repo>/shaders`, as the build's copy is only updated on build); a failed edit keeps the previous linked program, material values survive reloads, and errors go to `hpl.log` |
| `OVERTURE_BITMAP_FONTS=1` | Draw the game's bitmap fonts instead of the fonts in `<exe dir>/fonts/` |
| `OVERTURE_DEBUG_OVERLAY=1` | Start with the debug overlay shown (frame and GPU section times, draw calls); F3 toggles it in the game |
| `OVERTURE_REFVIEW=1` | Start with the reference view panel open (F2 toggles it in the game): go to the places the reference images are taken from |
| `OVERTURE_GL_DEBUG=1` | Create a debug OpenGL context and log driver messages to `hpl.log` |
| `OVERTURE_DUMP_FRAMES=60,300` | Save these frame numbers as PNGs (to `OVERTURE_DUMP_DIR`, default: game dir) |
| `OVERTURE_SMOKE=1` | Smoke test: load every map (and the test map), run, save, reload, run, quit. `OVERTURE_SMOKE=office,worm` limits it to maps whose names contain those words. Logs `SMOKE:` lines to `hpl.log`; exit code 1 on failure |
| `OVERTURE_MAP=name[:start]` | Skip the menus and load this map, e.g. `testmap` or `level01_05_office:link01` |
| `OVERTURE_PHYSLOG=1` | Log physics events (`PHYS:` lines) to `hpl.log`: bodies waking, resting, freezing; player grabs and releases |
| `OVERTURE_SMOKE_FRAMES` | Logic frames per smoke-test step (default 120, two seconds) |
| `OVERTURE_REFSHOTS=1` | Save reference images of every map and quit (`OVERTURE_REFSHOTS=office,worm` for some maps) to `OVERTURE_REFSHOTS_DIR`. Logs `REFSHOTS:` lines; exit code 1 on failure. Use `tools/refshots.py`, which runs it headless |
| `OVERTURE_EFFECTS_CAPTURE=dir` | Capture and check player feedback in `effectslab`; use `tools/refshots.py effects dir` for an isolated headless run |

## Requirements

- OpenGL 4.3 core, or 3.3 core with `GL_ARB_texture_storage` and
  `GL_ARB_texture_storage_multisample`
- 64-bit Linux or Windows

## Repository layout

```
engine/    HPL1 engine (FrictionalGames/HPL1Engine), ported
game/      Penumbra: Overture game code (FrictionalGames/PenumbraOverture), ported:
  core/      init, main loop, maps, saves, scripts, input bindings
  player/    player, its states and HUD models
  enemies/   enemy AI
  entities/  map objects and areas
  systems/   effects, fades, music, messages, radio, triggers, attacks, physics log
  ui/        menus, inventory, notebook, credits
shaders/   GLSL shaders (from ScummVM's HPL1 port); SMAA.hlsl is the SMAA
           reference implementation
testmap/   generators and level scripts for physics and player effects labs
tools/     refshots.py: reference images and player effects checks
vendor/    newton, angelscript, oalwrapper, tinyxml, glad, imgui, smaa
run.sh     launcher
```

## Credits and licences

- **Frictional Games**: HPL1 engine and Penumbra: Overture source code (GPLv3),
  shaders (zlib), OALWrapper (zlib)
- **The ScummVM project**: GLSL ports of the HPL1 shaders, and the Newton 2.36
  copy and porting work used as the reference for the physics port (GPLv3+)
- **Julio Jerez**: Newton Game Dynamics 2.36 (zlib)
- **Andreas Jönsson**: AngelScript 2.38 (zlib)
- **Lee Thomason and Yves Berquin**: TinyXML (zlib)
- **Omar Cornut and contributors**: Dear ImGui 1.92.9b (MIT)
- **glad** OpenGL loader by David Herberth (MIT / Apache-2.0 for the
  generated code)
- **Jorge Jimenez, Jose I. Echevarria, Belen Masia, Fernando Navarro and
  Diego Gutierrez**: SMAA, the reference shader (`shaders/SMAA.hlsl`) and its
  area and search textures (`vendor/smaa`) (MIT)

OpenHPL1 is licensed under the GNU GPL v3; see [COPYING](COPYING).
Bundled third-party code keeps its own licence (see each directory under
`vendor/`, `engine/LICENSE-shaders` and the head of `shaders/SMAA.hlsl`). Penumbra: Overture game assets are
not covered by this licence and are not included.
