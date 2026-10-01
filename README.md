# Overture Remade

A modernised source port of **Penumbra: Overture** (Frictional Games, 2007),
built from the open-source HPL1 engine and game code. It runs natively on
64-bit Linux against the game data from your own copy of the game.

The goal is to keep the game playing exactly like the original while bringing
the engine up to date: native widescreen, a modern renderer and a long list of
quality-of-life fixes. See [ROADMAP.md](ROADMAP.md).

> You need a copy of Penumbra: Overture (e.g. from Steam). This repository
> contains no game assets.

## Status

**Milestone 0 done:** the game builds and runs as a native 64-bit Linux
executable. It boots, plays the intro and menus and loads levels at full speed.
Full playthroughs are not tested yet, and physics behaviour still has to be
compared against the original.

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

Newton, AngelScript, OALWrapper and the glad OpenGL loader are vendored in
`vendor/`.

FreeType renders the fonts. With `freetype-devel` installed the system library
is used; without it, CMake downloads FreeType 2.13.3 and builds it into the
engine. The fonts themselves are in `fonts/`.

```
cmake --preset default
cmake --build --preset default
```

This produces `build/overture`, with the shaders copied to `build/shaders/`.
Other presets: `debug` (`build-debug/`) and `asan` (`build-asan/`, Debug with
AddressSanitizer and UBSan; needs `libasan libubsan`).

Static analysis uses `clang-tools-extra`. `cmake --build build --target tidy`
runs the checks in `.clang-tidy` over the engine and game sources, and
`git clang-format` formats changed lines.

Before changing gameplay code, run the smoke test on the ASan build:

```
cd "<game dir>" && OVERTURE_SMOKE=1 ASAN_OPTIONS=detect_leaks=0 /path/to/build-asan/overture
```

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

A capture (about a minute) visits every map: each start position, and up to
six of the map's lights, looking in four directions from each, with the
flashlight on. It saves the 3D view only, without the HUD, fades or menus,
at 1280x720 with fixed graphics settings. `--maps office,worm` limits it to
some maps, `--size` changes the resolution and `--set Graphics.Shadows=2`
any setting. `compare` lists the frames that differ and exits with 1 if any
do.

Two captures of the same build are identical except for a few pixels in a
few frames, which `compare` ignores (`--tolerance`, `--max-pixels`), and
possibly for `level01_19_door_shaft` and `level01_20_base_entrance`, whose
scripted light sequences are timed partly by sounds. Their differences are
listed as "unstable" and do not count.

## Running

```
./run.sh
```

`run.sh` changes into the game data directory and starts the executable. By
default it looks in the Steam location
`~/.local/share/Steam/steamapps/common/Penumbra Overture`; set
`OVERTURE_GAME_DIR` to use another install.

Settings, saves and the log (`hpl.log`) are stored in
`~/.frictionalgames/Penumbra/OvertureRemade/`. The original game's folder
(`.../Penumbra/Overture/`) is never read or written, so you can still play the
original side by side. Copy your old `save/` folder over if you want your
saves.

### Environment variables

| Variable | Purpose |
|---|---|
| `OVERTURE_GAME_DIR` | Game data directory used by `run.sh` |
| `OVERTURE_DISPLAY` | Monitor index (0-based) to open on, instead of the "Display" option; that defaults to the largest monitor |
| `OVERTURE_SHADER_DIR` | Load shaders from here instead of `<exe dir>/shaders/` |
| `OVERTURE_BITMAP_FONTS=1` | Draw the game's bitmap fonts instead of the fonts in `<exe dir>/fonts/` |
| `OVERTURE_GL_DEBUG=1` | Create a debug OpenGL context and log driver messages to `hpl.log` |
| `OVERTURE_DUMP_FRAMES=60,300` | Save these frame numbers as PNGs (to `OVERTURE_DUMP_DIR`, default: game dir) |
| `OVERTURE_SMOKE=1` | Smoke test: load every map (and the test map), run, save, reload, run, quit. `OVERTURE_SMOKE=office,worm` limits it to maps whose names contain those words. Logs `SMOKE:` lines to `hpl.log`; exit code 1 on failure |
| `OVERTURE_MAP=name[:start]` | Skip the menus and load this map, e.g. `testmap` or `level01_05_office:link01` |
| `OVERTURE_PHYSLOG=1` | Log physics events (`PHYS:` lines) to `hpl.log`: bodies waking, resting, freezing; player grabs and releases |
| `OVERTURE_SMOKE_FRAMES` | Logic frames per smoke-test step (default 120, two seconds) |
| `OVERTURE_REFSHOTS=1` | Save reference images of every map and quit (`OVERTURE_REFSHOTS=office,worm` for some maps) to `OVERTURE_REFSHOTS_DIR`. Logs `REFSHOTS:` lines; exit code 1 on failure. Use `tools/refshots.py`, which runs it headless |

## Requirements

- OpenGL 4.1 compatibility profile (or GL_ARB_separate_shader_objects); any
  GPU from the last decade
- Linux on x86-64 (Windows support is planned)

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
shaders/   GLSL shaders (from ScummVM's HPL1 port)
testmap/   generator and level script for the physics test map
tools/     refshots.py: reference images for renderer changes
vendor/    newton, angelscript, oalwrapper, tinyxml, glad
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
- **glad** OpenGL loader by David Herberth (MIT / Apache-2.0 for the
  generated code)

Overture Remade is licensed under the GNU GPL v3; see [COPYING](COPYING).
Bundled third-party code keeps its own licence (see each directory under
`vendor/` and `engine/LICENSE-shaders`). Penumbra: Overture game assets are
not covered by this licence and are not included.
