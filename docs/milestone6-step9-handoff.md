# Milestone 6, step 9 handoff

Continue Overture Remade with ROADMAP step 9: split `cLowLevelGraphicsSDL`
into classes for its window/context, GL state, scene/output framebuffers,
final passes, shadow-map targets and debug drawing responsibilities.

Workspace: `<workspace>`.
Read applicable `AGENTS.md` instructions, inspect the working tree and read
`ROADMAP.md` first. Preserve subsequent changes. No applicable AGENTS files
were found during step 8. The step 8 commit includes this handoff; identify
it with `git log` rather than assuming the working tree is unchanged.

## Scope

This is a structural refactor that preserves the picture and renderer
behaviour. Inspect callers before choosing ownership and interfaces.
Keep `iLowLevelGraphics` as the renderer-facing contract unless inspection
shows a concrete reason to change it. Work in coherent stages that can be
built and checked independently. Avoid replacing one large class with
helpers that still depend on its entire private state.

Leave the broader billboard, beam and particle rewrite for step 10. They
still construct/update geometry on the CPU, already using triangles and
the step 8 upload layout. Do not restore compatibility pipelines, matrix
stacks, quad drawing or legacy texture-coordinate slots.

Update `README.md`, `ROADMAP.md` and `CHANGES.md` with the actual final
implementation and validation. Mark step 9 complete only after verification.
Leave step 9 uncommitted unless instructed otherwise.

## Current renderer contracts

- `cDrawMatrices` supplies model, view and projection together. `eMatrix`
  and the matrix stacks are gone.
- `CreateProgram` takes GLSL names, explicit defines and an optional
  different vertex shader. Programs link both stages. Some older backend
  comments still mention a pipeline; the implementation uses linked pairs.
- `eProgramParam` values live in each program's `hpl_Material` UBO at
  binding 2. Frame and state blocks use bindings 0 and 1. Dirty material
  values upload before drawing.
- `cGLSLProgram::Apply(arrayMask, layers)` selects a linked sampler layout.
  Static maps use sampler2D, animated maps sampler2DArray, on the same unit.
  Only active 2D samplers participate in the layout mask. Layer locations
  resolve at link time, and layer values update even when material values
  are clean. `PrepareDraw()` returns false if a usable layout is unavailable;
  callers skip that draw.
- Both shader stages and includes are watched. Reload rebuilds every used
  sampler layout atomically, retaining the full working set on failure and
  preserving material values. `shaders/hpl1_texture.glsl` supplies numbered
  sampler declarations and sampling/LOD/fetch/size helpers.
- Engine textures use units 0–7. Shadow comparison/depth use 8/9; scene depth
  uses 10 and scene colour 11. Preserve these bindings and sampler state.
- Flat, stable draw queues use lexicographic keys. `cSpriteBatch` merges
  adjacent sprites by texture/blend and emits triangles. GUI batches flush
  before clipping changes and retain caller matrices, depth and culling,
  including 3D GUI sets.
- Equal-Z HUD/GUI sprites retain submission order. This deliberately keeps
  inventory borders beneath the keypad. Preserve the correction.
- `cVertexBufferVBO` gathers packed CPU editing arrays into one interleaved
  GPU vertex buffer and uses an element buffer for indices. CPU arrays keep
  their packed per-attribute strides for skinning, physics, bounds and
  caches. Partial updates gather all enabled fields; copies own their data.
- Named attributes are position (location 0), normal (1), colour (2),
  `hpl_TexCoord` (4), and four-component `hpl_Tangent` (5). There are no
  `hpl_MultiTexCoord0–4` slots or quad buffer draw type. Debug lines remain.
- Skyboxes create triangle indices directly. Collada polygon corners are
  triangulated before splitting vertices; `GeometryVersion=2` invalidates
  older geometry caches. MSH positions are xyz on disk and xyzw in memory;
  attribute export uses each field's own element count.
- Resource, float, scene, resolve, output, multisample and shadow textures
  use immutable storage. GL 4.3 suffices; earlier core contexts (3.3+) need
  both ARB_texture_storage and ARB_texture_storage_multisample. The SDL init
  path explicitly loads their entry points on older contexts.
- Atlas updates reuse storage. A dimension/format change creates a new GL
  object, retaining the old storage if allocation fails. `TextureStorageChanged`
  refreshes every cached unit binding; `SetRenderTarget` reattaches a resized
  resource even if its pointer is unchanged. Preserve both behaviours.
- Animation frames occupy layers of one texture array. Loop/oscillate,
  fractional time and manual stepping retain their old timing. Mipmaps,
  quality averaging, filtering/wrapping and per-frame alpha coverage remain.
  Coverage reads each whole array mip once, with separate coverage targets
  for its layers.

## Inspect first

- `engine/include/impl/LowLevelGraphicsSDL.h`
- `engine/sources/impl/LowLevelGraphicsSDL.cpp`
- `engine/include/graphics/LowLevelGraphics.h`
- `engine/sources/impl/SDLTexture.cpp`
- `engine/sources/impl/VertexBufferVBO.cpp`
- `engine/sources/impl/GLSLProgram.cpp`
- `engine/sources/impl/DebugOverlay.cpp` and `GpuProfiler.cpp`
- `engine/sources/graphics/Renderer3D.cpp` and `RendererPostEffects.cpp`
- `engine/sources/graphics/SpriteBatch.cpp`
- `tests/engine_regressions.cpp` and `tests/renderer_storage.cpp`

The backend currently owns context/display selection and capture, cached
GL state and texture/sampler bindings, scene/resolve/output framebuffer
lifetime, fullscreen program passes and temporary state restoration,
SMAA/output/gamma/player-effect presentation, shadow targets and restoration,
and streamed triangle/sprite/debug geometry. Extract ownership and lifetime
along those responsibilities. Audit initialization/destruction order,
context availability, render-scale/FSAA resizing, scene resolve order,
framebuffer/viewport restoration, and raw GL changes that interact with caches.

The EGL regression creates an external surfaceless context, then constructs
`cLowLevelGraphicsSDL` without calling its SDL window initialization. Preserve
that useful separation while moving resource ownership.

## Validation and current baselines

Run:

```sh
cmake --build --preset default -j 8
ctest --test-dir build --output-on-failure
```

Step 8 passes both Linux suites: `engine_regressions` and the optional
`renderer_storage` EGL suite, which actually ran on Mesa llvmpipe
(LLVM 21.1.8). EGL headers/library enable the latter; lack of a surfaceless
GL 4.3 context is a CTest skip. It checks real storage, mipmaps, atlas updates,
array layers/timing/coverage/quality/samplers, resized FBO attachments,
multiple cached texture units, material sampler layouts and valid/invalid
include reloads. Engine tests include skinning, packed/interleaved uploads,
mesh copies, tangent handedness and Collada polygon/cache checks.

Use `tools/refshots.py` without changing comparison tolerances. Linux
headless game captures were unavailable because gamescope is missing.
Windows captures ran on an RTX 2060, GL 4.6. Current picture in `/tmp`:

- `/tmp/step8-after`: 95 default frames.
- `/tmp/step8-after-motion`: 95 motion-blur frames; GL debug enabled.
- `/tmp/step8-after-scale`: 31 frames, filtered to
  `office,water_cave,icecave,outside`, Render Scale 1.5, 4× FSAA.
- `/tmp/step8-after-effects`: 29 effects-lab frames; pixel and live-transition
  checks pass, GL debug enabled.

Against step 7: default matches 93/95, motion 95/95, scale 29/31, effects
29/29. Only icecave-view01/04 vary: default 106/837 pixels, scale 114/727.
This variation was reproduced in unchanged builds before steps 7 and 8.
All 2D frames match step 7. Motion/effects debug runs have no GL errors or
shader/material-layout errors, only synchronized pixel-readback performance
warnings. The vertex-only step 8 stage `/tmp/step8-vertices` matches 95/95
step 7 default frames. Capture fresh baselines if necessary.

Final logs are `/tmp/step8-final-linux-build.log`,
`/tmp/step8-final-linux-ctest.log`, `/tmp/step8-final-windows-build.log`, and
`/tmp/step8-{default,motion,scale,effects}-{capture,compare}.log`.

## Windows test copy

Path: `<windows-workspace>`.
It contains step 8 renderer changes plus Windows/platform/capture additions
absent from Linux. Apply focused patches; do not overwrite its tree wholesale.
In particular it has hidden-window support, `CaptureNextFrame`,
`ReadFramePixels` and comparison-image capture hooks in the backend.
Filesystem writes/builds/captures may require sandbox escalation.

Build with that copy as the working directory:

```text
cmd.exe /c platform\windows\build.cmd -j 6
```

Its MSVC kit disables CTest targets. Report Linux regressions separately
from Windows build/capture validation. The step 8 optional test/CMake changes
and Linux documentation were not copied wholesale into this test copy.

Game data:
`/mnt/c/Program Files (x86)/Steam/steamapps/common/Penumbra Overture/redist`.
Run capture jobs separately; a combined long-running shell previously ended
after two captures. Use separate output directories and wait for each job.

```sh
python3 tools/refshots.py capture /tmp/step9-after --windows \
  --build <windows-workspace>/build-windows \
  --game-dir '/mnt/c/Program Files (x86)/Steam/steamapps/common/Penumbra Overture/redist'
```

Motion adds `--set Graphics.MotionBlur=true`. Scale adds
`--maps office,water_cave,icecave,outside --set Graphics.RenderScale=1.5 --set Graphics.FSAA=4`.
Effects uses `python3 tools/refshots.py effects OUT_DIR` with the same Windows,
build and game-dir arguments. Set `OVERTURE_GL_DEBUG=1` for debug validation.
Compare each result against the corresponding step 8 directory with
`python3 tools/refshots.py compare BASELINE NEW --diff-dir DIFF_DIR`.
