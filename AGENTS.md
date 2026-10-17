# Working in OpenHPL1

## Scope and first steps

OpenHPL1 is a C++17 port of Frictional Games' HPL1 engine and Penumbra:
Overture, with experimental Black Plague and Requiem game code. One executable,
`openhpl1` (`openhpl1.exe` on Windows), uses assets from an installed game.
The repository does not include those proprietary assets.

- Read `README.md` for commands, `ROADMAP.md` for current work and
  `docs/README.md` for subsystem guides. `CHANGES.md` is implementation history.
- Inspect `git status` and relevant diffs before editing. Preserve existing
  work, including untracked files; do not reset, discard, stage or commit
  unrelated changes.
- `docs/archive/` contains historical plans, reports and handoffs. Their
  assignments, old paths and old renderer descriptions are not instructions
  for the current tree.

## Source layout

- `engine/include/`, `engine/sources/`: shared engine; corresponding subsystem
  directories hold headers and implementations. `impl/` contains SDL3/GL,
  Newton, audio and platform backends.
- `game/core/`: startup/game selection, maps, saves, scripts and input.
  `player/`, `enemies/`, `entities/`, `systems/` and `ui/` hold game behaviour.
- `shaders/`: GLSL programs/includes; `hpl1_texture.glsl` handles ordinary and
  animated-array texture sampling. `SMAA.hlsl` is third-party reference code.
- `testmap/`: source generators/scripts for physics, Dog and effects labs.
  Generated maps belong in build directories.
- `tests/`: engine regressions, EGL renderer checks and opt-in in-game Dog tests.
- `tools/`: captures/checks and porting diagnostics. `platform/windows/` holds
  build/run/smoke/sync scripts. `vendor/` contains third-party dependencies;
  avoid unrelated edits there and preserve licences.

## Build and run

Use the existing CMake presets and launcher. Linux dependencies are listed in
README; FreeType uses the system installation or a CMake download fallback.

```sh
cmake --preset default
cmake --build --preset default
ctest --test-dir build --output-on-failure
```

`debug` uses `build-debug/`; `asan` uses `build-asan/` with AddressSanitizer
and UBSan. `BUILD_TESTING` defaults on. EGL headers/library enable the optional
`renderer_regressions` target; CTest can skip it if no usable context is available.
A skipped or absent target is not a renderer test pass.

```sh
cmake --preset asan
cmake --build --preset asan
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-asan --output-on-failure
```

Disabling leak detection is used for graphics-driver teardown/restricted
environments; record that limitation and do not claim leak-free validation.
Keep existing sanitizer policy: AngelScript's packed argument alignment is
excluded, and vendored Newton retains ASan but excludes UBSan.

Run `./run.sh`, `./run.sh blackplague` or `./run.sh requiem` after building.
`OVERTURE_GAME_DIR` overrides the launcher's Steam game-data location. The
launcher runs `build/openhpl1` from the data directory; another build must be
launched explicitly from that directory or through a tool's `--build` option.

Windows uses `platform\windows\build.cmd` with Visual Studio's C++ tools and
vcpkg (`VCPKG_ROOT`), then `run.cmd` or `smoke.cmd`. CMake disables the CTest
targets under MSVC; report Windows build/smoke separately from Linux tests.

## Choose validation for the change

Documentation-only edits need link/path and diff checks. For code changes,
build and run relevant existing regressions; add focused coverage when fixing
a behaviour that those tests do not exercise. Run sanitizers for memory,
ownership, parsing, serialization, physics and animation changes.

- Gameplay/physics: run the ASan game smoke test with isolated settings and
  saves. `OVERTURE_SMOKE=1` loads maps, runs, saves, reloads and quits;
  substrings select maps and `OVERTURE_PHYSLOG=1` records body/joint activity.
  Compare physics-dependent mechanisms and changed enemy behaviour manually.
- Dog AI/animation: use
  `python3 tools/check_dog_foundations.py --build build-asan` and relevant
  arena/full-map variants from `docs/dog-foundations.md`. It needs gamescope,
  original game data and `BUILD_TESTING=ON`.
- Renderer changes: run `engine_regressions` and `renderer_regressions` through
  CTest. The latter uses surfaceless software EGL without game data or
  gamescope. Check both the GL 3.3 extension path and GL 4.3 core path when
  touching backend resources, shaders or context requirements.
- Rendering preservation: capture before and after with the same maps,
  settings and resolution. Check 2D screens too. Include motion blur,
  scaled/MSAA rendering and effects checks when those paths are affected:

  ```sh
  python3 tools/refshots.py capture /tmp/ref-before
  # Rebuild after the change, then:
  python3 tools/refshots.py capture /tmp/ref-after
  python3 tools/refshots.py compare /tmp/ref-before /tmp/ref-after --diff-dir /tmp/ref-diff
  python3 tools/refshots.py effects /tmp/effects
  ```

  See README for `--maps`, `--size`, `--set`, `--build` and Windows capture.
  Do not loosen comparison tolerances to hide regressions. Note intentional
  visual changes and known unstable maps separately. Use `OVERTURE_GL_DEBUG=1`
  for driver diagnostics.
- Performance: measure representative CPU/GPU frame times. Call counts,
  uploaded bytes and synthetic fixtures establish local changes, not game FPS.

Headless game tools need installed data and gamescope; inspect their actual
exit status and `SMOKE:`/check logs. If assets, display, drivers or toolchains
are missing, finish the available checks and state exactly what was skipped.

## Behaviour and renderer contracts

- Preserve gameplay, script signatures and save compatibility unless the
  task explicitly changes them. Shared engine changes can affect all three
  games. Use the existing game-selection checks (`cInit::IsOverture()` etc.)
  where behaviour differs. Keep `OVERTURE_*` and existing personal-directory
  names compatible despite the executable rename.
- Keep installed assets and normal user profiles untouched during automation.
  Prefer isolated tool runners or `OVERTURE_PROFILE_ROOT` (settings, saves,
  logs and writable caches). Normal personal directories are under
  `~/.frictionalgames/Penumbra/{Overture,BlackPlague,Requiem}Remade/` on Linux;
  Windows uses its Documents directory. Requiem uses Black Plague's data
  installation with the expansion present.
- The renderer has one improved look. Do not reintroduce fixed-function GL,
  matrix stacks, Cg-name translation, quads, stencil shadow volumes or removed
  Original/Improved rendering switches. Dog AI still has both choices.
- Support GL 4.3 core or GL 3.3 core with both `GL_ARB_texture_storage` and
  `GL_ARB_texture_storage_multisample`. Gate newer optional features and
  retain a supported path.
- Use explicit `cDrawMatrices`, linked GLSL programs/material uniform blocks,
  named vertex attributes, immutable textures and the shared sprite/instance
  paths. Preserve animated-array sampler layouts and atomic shader reloads:
  failure keeps the previous usable programs and material values.
- Keep GL state and resource ownership with the backend components
  (`cSDLGraphicsWindow`, `cGLGraphicsState`, `cGLRenderTargets`,
  `cGLFinalPasses`, `cGLShadowTargets`, `cGLStreamDrawing`). Restore targets,
  viewport and tracked state around temporary passes; release GL resources
  before the context. Raw external GL clients must observe the limited
  `ResyncExternalState()` contract and restore their own bindings/functions.
- Preserve transparent ordering and equal-depth submission order. Invalidate
  resolved depth only on real depth writes. Follow `docs/shadow-cache.md` for
  geometry/texture/shader revisions, live inputs and per-face cache validity;
  raw texture writers must call `MarkContentChanged`.
- Keep the frame order: scene → post effects → output transform → HUD/UI →
  gamma → window. Do not let scene effects alter the HUD.

## Style and documentation

Follow surrounding Frictional Games conventions (`cFoo`, `iFoo`, `mpMember`,
typed argument prefixes). `.clang-format` uses tabs, Allman braces and
`if(...)` without a space. Format only changed lines; avoid tree-wide style
or naming churn. `.clang-tidy` defines bug-finding checks; the optional
`cmake --build build --target tidy` target requires clang-tools-extra. Review
findings rather than applying broad automatic fixes.

For behaviour or architecture changes, update the relevant current guide and
README as needed, record implementation and validation in CHANGES, and adjust
ROADMAP when status changes. Keep one current roadmap. Archive superseded
plans with context, redirect references and carry forward unresolved work.
Distinguish code implemented, tests passed, visual checks, manual play and
measured performance; do not report an unavailable check as passed.
