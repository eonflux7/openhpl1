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
| `OVERTURE_DISPLAY` | Monitor index (0-based) to open on; default is the largest monitor |
| `OVERTURE_SHADER_DIR` | Load shaders from here instead of `<exe dir>/shaders/` |
| `OVERTURE_GL_DEBUG=1` | Create a debug OpenGL context and log driver messages to `hpl.log` |
| `OVERTURE_DUMP_FRAMES=60,300` | Save these frame numbers as PNGs (to `OVERTURE_DUMP_DIR`, default: game dir) |

## Requirements

- OpenGL 4.1 compatibility profile (or GL_ARB_separate_shader_objects); any
  GPU from the last decade
- Linux on x86-64 (Windows support is planned)

## Repository layout

```
engine/    HPL1 engine (FrictionalGames/HPL1Engine), ported
game/      Penumbra: Overture game code (FrictionalGames/PenumbraOverture), ported
shaders/   GLSL shaders (from ScummVM's HPL1 port)
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
