# Shadow-map cache

The renderer retains depth maps per light, with a default 128 MiB budget and
at most 64 entries. The F4 Shadows window exposes the budget (zero disables
reuse) and the number of faces reused in the last frame. The budget accounts
for four bytes per depth texel plus the CPU input signature; driver allocation
overhead is not measurable here. The original uncached spotlight/cube scratch
targets are separate. Maps larger than the budget use those scratch targets.

`cRenderer3D` assembles an exact input signature, without a hash, after visible
and offscreen shadow-caster graphics have been updated. It includes the light
position/projection, rasterized sides, alpha-caster and static-only modes,
caster membership and eligibility, model matrices, bounds, geometry revisions,
alpha texture revisions/frames/sampling, and the shadow shader revision.
Sampling-only shadow settings (bias, softness, sample counts) do not change
stored depth. Their cube-face visibility margin is still evaluated every frame.

Runtime light IDs are independent of addresses and save IDs. Geometry, texture
and shader revisions distinguish replacement resources as well as updates.
Snapshots contain values, not references to objects from earlier frames.
Visibility, shadow-casting and material changes are rechecked for retained
caster lists, including submesh parents and forced shadows.

`cGLShadowTargets::SelectShadowMap` binds the selected texture on reserved units
8/9 and returns its valid-face mask. Each cube face becomes valid only after
rendering completes; a failed shader application leaves the face invalid so
the next frame retries. A camera turn renders newly exposed faces without
discarding the others. Selection and cache hits leave the framebuffer and
viewport alone. Misses use the existing shadow-target transitions and restore
the scene/resource target afterwards. Scene resize and MSAA do not invalidate
independent shadow textures.

Eviction uses least-recently-used order. Compatible evicted storage is reused
when the light working set exceeds the budget. Changing caster membership
updates the signature and validity without reallocating an unchanged texture
layout. Shadow framebuffer attachments are detached after rendering, so they
cannot keep evicted storage alive. Reset releases retained and scratch textures
and samplers before the GL context is destroyed.

CPU geometry edits follow the existing `UpdateData` contract. Texture uploads,
alpha-coverage edits and sampler changes advance revisions. Raw texture writers
must call `MarkContentChanged`. Live render-target textures and alpha casters
that inherit colour/UV constants use scratch maps because their inputs may
change outside those revision boundaries. Animated CPU skinning still uploads
each changed frame and therefore invalidates its shadows; unchanged sleeping
poses can reuse them.

The EGL regression checks storage isolation, budgets, eviction/recycling,
per-face validity and teardown. It also executes production spotlight and point
shadow rendering with GPU primitive queries and depth readbacks. It compares
cached/uncached depth and exercises movement, geometry and element changes,
visibility/caster membership, projection/settings, alpha edits/animation,
implicit attributes, live targets, shader failures/reloads and scene resizing.

This reduces shadow draws in the regression fixtures. CPU signature cost and
full-game CPU/GPU frame-time effects still need measurement on representative
game scenes. Full-game visual captures and Windows validation are separate
from the EGL tests.

## Validation, 2026-10-18

The normal build and both regression suites pass. Both suites also pass with
ASan/UBSan, with leak detection disabled. EGL passes on llvmpipe under GL 3.3
and GL 4.3, including the generated-primitive and exact depth comparisons.

```sh
cmake --build --preset default -j 8
ctest --test-dir build --output-on-failure
cmake --build --preset asan --target engine_regressions renderer_regressions -j 8
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-asan --output-on-failure
LIBGL_ALWAYS_SOFTWARE=1 MESA_GL_VERSION_OVERRIDE=3.3 MESA_GLSL_VERSION_OVERRIDE=330 ./build/renderer_regressions
LIBGL_ALWAYS_SOFTWARE=1 MESA_GL_VERSION_OVERRIDE=4.3 MESA_GLSL_VERSION_OVERRIDE=430 ./build/renderer_regressions
```

Logs: `/tmp/openhpl1-shadow-cache-{build-final,tests-final,asan-build,asan-tests,gl33,gl43}.log`.
The pre-change CTest baseline also passed (`/tmp/openhpl1-shadow-cache-baseline.log`).
`python3 tools/refshots.py capture /tmp/openhpl1-shadow-cache-captures` stopped
at the missing game-data check. No full-game captures, Windows build or game
frame-time measurement is claimed.

## Follow-up validation, 2026-10-19

Fresh Windows/MSVC builds and current Overture smoke/save/reload pass.
Same-driver captures against pre-step-11 `c2ea1a1` match all UI and effects
frames, with unresolved boat-cabin and ice-cave scene differences. A separate
budget-zero diagnostic matches all 19 cached first-map/UI frames; disabling
reuse does not remove the boat-cabin mismatch. This is limited capture
coverage, not a comprehensive in-game invalidation or performance check.
Intermittent startup access violations also affected baseline and current.
See [CHANGES](../CHANGES.md#renderer-steps-1113-game-validation-2026-10-19)
for results, failed attempts and artifact paths, and the
[roadmap](../ROADMAP.md#validation-to-close-first) for remaining scenarios
and measurements. No game frame-time improvement is claimed.
