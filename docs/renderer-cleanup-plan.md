# Renderer cleanup implementation plan

Prepared 2026-10-17 for a worker implementation followed by independent review.

## Assignment

Implement stages 1–6 below in order, with a buildable checkpoint for each
stage. Preserve the current improved look except for the explicit transparent
ordering correction and rendering lights previously dropped by the 30-light
limit. Stage 7 is a profiling investigation; GPU skinning is a separate follow-up,
not a prerequisite for completing this assignment.

Read the current source and applicable `AGENTS.md` instructions before editing.
This plan describes the inspected tree, not an API that must be implemented
literally. Adjust implementation details when caller tracing demonstrates a
better choice, and record material departures in the handoff.

Do not commit or publish changes unless asked. Do not overwrite unrelated work.
At planning time, the tree already contains changes to README, CHANGES, CMake,
game initialization, launch/build/capture scripts, and the Windows manifest.
Inspect `git status` and preserve those changes; do not restore whole files.

## Contracts to retain

- GL 3.3 core with the existing immutable-storage extensions remains supported;
  GL 4.3 must work without requiring GL 4.5. Do not introduce a newer mandatory
  API or replace the renderer/backend wholesale.
- Keep frame order: scene → post effects → output transform → HUD → gamma →
  window. Retain linear lighting, alpha matching, alpha-to-coverage, and current
  tuning defaults.
- Preserve linked shader variants, animation array layers, atomic shader reload,
  and uniform block bindings 0/1/2. Engine textures use units 0–7; shadows use
  8/9, scene depth 10, and scene colour 11.
- Retain explicit draw matrices, interleaved mesh uploads, packed CPU editing
  arrays, instanced billboards/beams/particles, and stable HUD/GUI submission
  order. Materials named `*2D` still serve sprites; their names alone are not
  evidence that they are dead.
- Preserve resource-target and shadow transitions, viewport restoration,
  immutable-texture resize notifications, and release of GL resources before
  context destruction. EGL tests initialize resource owners without an SDL
  window; retain that separation.
- Keep changes focused. Do not also implement clustered lighting, shadow-map
  caching, an ECS, a general render graph, or a new graphics API.

## Stage 0: establish fresh baselines

1. Record the starting revision and working-tree diff. Read Milestone 6 in
   `ROADMAP.md` and the current renderer sections in README and CHANGES.
2. Build and run the suites from the current tree:

   ```sh
   cmake --build --preset default -j 8
   ctest --test-dir build --output-on-failure
   ```

3. Capture fresh before images with `tools/refshots.py`: defaults, motion blur,
   Render Scale 1.5 with 4× FSAA, and the effects lab. Use the current documented
   Linux or Windows workflow and record exact commands/settings. Use distinct
   directories outside the source tree for each checkpoint. Run GL debug output
   on motion/effects captures.
4. If game data, a display runner, or Windows tooling is unavailable, continue
   implementation and executable regression testing. Report visual/platform
   validation as unavailable, never as passed. Do not loosen image tolerances.

The planning review ran both existing CTest binaries successfully, without a
rebuild or visual captures. That result is not a fresh implementation baseline.

## Stage 1: correct transparent ordering

Primary files: `engine/sources/graphics/RenderList.cpp`,
`engine/include/graphics/RenderList.h`, `tests/engine_regressions.cpp`.

The comparator around `RenderList.cpp:56` compares A's depth-test flag against
A's flag, so its depth-test branch never runs. Transparent objects are inserted
into their multiset by iterating a pointer-ordered object set; equal depths
therefore inherit allocation order rather than original submission order.

1. Trace callers and document the chosen depth-test grouping. For the small
   correctness patch, repair A-versus-B and preserve the comparator's existing
   `<` policy (depth-disabled before depth-enabled), unless a concrete caller
   requires a different policy. Any departure is an additional intentional
   behavior change and needs a focused test and explanation.
2. Preserve transparent submission order when objects first enter the render
   list, before traversing `m_setObjects`. Use a retained contiguous collection
   and stable sorting, or explicit submission ordinals. Continue deduplicating
   objects through the render-count mechanism.
3. Within each depth-test group, retain ascending camera-space Z, which is
   back-to-front for the current camera convention. Equal-depth draws retain
   submission order. Do not use material, texture, buffer, or object addresses
   as transparency tie-breakers.
4. Test actual render-list behavior: mixed depth-test flags, distinct depths,
   equal depths, reversed allocation versus submission order, duplicate adds,
   and clear/rebuild across frames. Do not only test a replacement comparator
   detached from the production submission path.

Acceptance: ordering is explicit and covered. Capture this behavior correction
as its own baseline before the structural transparent rewrite in stage 4.
Stable sorting only guarantees preservation of the supplied submission order;
do not claim all upstream scene traversal is deterministic without checking it.

## Stage 2: remove confirmed dead renderer interfaces

Primary files: `Material.h`, `Material_*.h/.cpp`, `RenderList.h/.cpp`,
`RenderState.h/.cpp`, and their callers under `engine/` and `game/`.

1. Remove the unused `GetTextureBlend()` virtual method, eight overrides, and
   definitions after confirming there are still no callers. This is an obsolete
   per-texture blend interface; live per-draw blend modes remain.
2. Remove `mTransQueue` and `eRenderListDrawType_Trans` if still unused. Simplify
   `GetQueue()` and `AddDraw()` accordingly. Stage 4 will introduce a real ordered
   transparent representation rather than retaining this empty placeholder.
3. Trace the unused `abUseDepth` argument, `mfZ` render-state field, and empty
   depth-state application. Remove only the portions proven unreachable or
   meaningless; keep live pass/channel state and actual transparent Z sorting.
4. Search the full repository for each removed symbol and build all normal
   targets. Adapt meaningful queue-transition tests if state-key boundaries
   change; do not add tests asserting only that deleted symbols are absent.

Acceptance: no dangling references or replacement compatibility stubs, no
picture changes, and no deletion of live sprite/image/material paths.

## Stage 3: replace the fixed light capacity

Primary files: `GraphicsTypes.h`, `RenderList.h/.cpp`, `Renderer3D.cpp`.

`MAX_NUM_OF_LIGHTS` is 30. Compile builds fixed per-light arrays, and RenderLight
stops after the first 30 lights in a pointer-ordered set.

1. Build one explicit per-frame light list. Each entry owns or references its
   light, draw queue, and affected-object count. Compile and RenderLight must
   consume that same list; remove the parallel-iterator/index assumption.
2. Grow storage to the visible light count and reuse allocations across frames.
   Remove the fixed arrays, macro, and truncation checks. Do not replace them
   with another silent cap or a new light-selection heuristic.
3. Preserve intersection tests, sector restrictions, empty-light skipping,
   debug filtering, shadow settings, and per-light material passes. Keep the
   existing iteration order for previously supported scenes unless a tested
   reason requires a change; light-order determinism can be separate work.
4. Test 0, 1, 30, 31, and at least 64 lights through production list compilation.
   Include lights that affect no objects, different intersections per object,
   sector restrictions, and a many-light → few-light → many-light transition.
   Verify queue-to-light association and reset counts, not merely vector size.

Acceptance: every eligible light has its own correct queue, including beyond
30; existing scenes retain their picture. Extra lights naturally cost more;
do not promise this change improves GPU performance.

## Stage 4: make transparent drawing explicit

Primary files: `Renderer3D.cpp`, `RenderList.h/.cpp`, `RenderState.h/.cpp`,
`Renderable.h`, and billboard/beam/particle code only where needed.

`RenderTrans()` currently mixes roughly 500 lines of draw-state management,
refraction, alpha masks, scene copies, and custom object callbacks.

1. Introduce ordered transparent draw records for the state/data actually
   needed: object, texture bindings, blend/alpha/depth state, matrices, geometry,
   program/setup, and custom-draw/refraction information. Reuse suitable opaque
   state application code without sorting transparency by opaque state keys.
2. Build records in the stage 1 order. Treat refraction scene copies, masks,
   and custom draws as ordered operations. Extract focused helpers where useful;
   do not replace the function with a helper carrying its entire former state.
3. Preserve when `GetTransDraw()` and `SetupTransDraw()` run. Audit whether
   callbacks mutate data or depend on current draw state before moving their
   execution. Do not call a stateful callback twice to discover its state.
   Keep material sorting policy explicit if a custom draw overrides depth test.
4. Preserve per-object refraction: clipped copy bounds and margins, destination
   alpha mask creation/clearing, standard-transparency skipping, special
   specular variants, and which earlier transparent draws the copy contains.
   Combining all refractive copies into one frame-wide copy changes behavior.
5. Preserve custom halo/shaft/particle programs, soft depth, animated layers,
   two-sided draws, setup callback boundaries, and following-draw state. Cached
   matrix pointers must remain valid until execution; copy values if needed.
6. Remove the superseded transparent multiset/manual state path once records
   execute every case. Keep only adjacent compatible optimizations that cannot
   change blending order. Do not force unrelated opaque queue redesign.

Acceptance: stage 1 order tests still pass; targeted render tests exercise
ordinary transparency → custom draw → refraction → ordinary transparency with
state transitions. Fresh image comparisons against stage 3 preserve the picture,
including water, particles, halos, shafts, fog, UI, and effects-lab transitions.
If shipped views miss a case, add a small targeted fixture rather than assuming
the full capture set exercises it.

## Stage 5: resolve scene depth only when needed

Primary files: `GLRenderTargets.h/.cpp`, `GLGraphicsState.h/.cpp`,
`LowLevelGraphicsSDL.cpp`, relevant draw entry points, and `renderer_storage.cpp`.

AO requests depth, transparency requests it again, and ResolveScene copies it
again. `mbSceneResolved` tracks the whole colour/depth transition, so it cannot
alone express whether the multisampled depth copy is current.

1. Track validity or a generation for the resolved scene depth independently
   of colour resolution. BeginScene and target recreation invalidate it.
2. Invalidate when a clear or actual draw can write the multisampled scene's
   depth attachment. Track destination, attachment presence, depth-test enable,
   depth-write mask, and shadow/resource targets correctly. Toggling the write
   mask is not itself a depth write; `PrepareDraw()` alone is not a completed
   draw. Conservative invalidation is acceptable where necessary, but must not
   erase the benefit for normal depth-disabled post effects.
3. Make repeated ResolveSceneDepth calls no-ops while the copy is current.
   ResolveScene still resolves colour and switches the scene destination as
   before, omitting depth only when it is already current. Preserve unresolved
   colour for AO/refraction and preserve partial colour blits for scene copies.
4. Do not invalidate scene depth for shadow maps, unrelated resource targets,
   colour-only passes, or scene copies. Audit debug rendering and game/custom
   draws after the first depth resolve; do not assume nothing ever writes depth.
5. Extend EGL tests to count real blits and inspect resulting pixels/depth:
   repeated reads use one depth blit, an intervening depth draw/clear forces
   another, and colour-only/offscreen/shadow work does not. Cover the final
   colour resolve, a new frame, render-scale/FSAA changes, scissor/write-mask
   restoration, and non-MSAA operation. Tests using raw GL writes must explicitly
   exercise the defined invalidation boundary.

Acceptance: normal AO + transparency + final resolve reuses unchanged depth,
and a genuine intervening depth write always becomes visible to readers.

## Stage 6: give fullscreen passes explicit state ownership

Primary files: `GLGraphicsState.h/.cpp`, `GLFinalPasses.cpp`,
`GLRenderTargets.cpp`, `GLShadowTargets.cpp`, `GLSLProgram.cpp`,
`DebugOverlay.cpp`, and `renderer_storage.cpp`.

DrawFullscreenPass currently queries program/UBO/texture bindings, capabilities,
masks, and viewport for every pass, then restores them manually.

1. Audit every raw mutation of the state the pass needs, including final passes,
   shadow/resolve helpers, texture creation, shader apply/reload, ImGui, and EGL
   fixtures. `cGLGraphicsState::Invalidate()` currently dirties shader-visible
   values only; it does not resynchronize a complete GL cache.
2. Add a small typed pass-state description/scope and a clear restoration
   contract. Use the state owner and target owner as the authoritative sources
   for state they actually track. Route engine mutations through them, or define
   explicit external-state invalidation/resynchronization at narrow boundaries.
   Never replace live GL queries with stale assumed defaults.
3. Preserve incoming capabilities, masks, viewport, active texture unit, texture
   target bindings, samplers, GL program, and material UBO binding as required by
   current callers. Keep actual bound program/variant and logical shader binding
   consistent; preserve animation-array bindings when a 2D texture shares a unit.
   Preserve the established VAO contract rather than silently changing it.
4. Remove hot-path queries only after the ownership audit makes each safe.
   Audit the six clip-distance save/restore entries: remove them only if all
   supported draws keep clipping disabled. Keep necessary queries at explicit
   external integration boundaries; a universal query ban is not the goal.
5. Expand the existing hostile-state restoration EGL tests, including a draw
   immediately after a fullscreen pass, shadows, resource targets, shader reload,
   array textures, and external/raw state changes under the chosen contract.
   Do not weaken existing restoration tests to accommodate an incorrect cache.

Acceptance: fullscreen passes retain their picture and caller-state contract;
normal repeated passes avoid the eliminated state queries. Provide before/after
call counts or a trace. Report measured frame/CPU/GPU effects separately; fewer
GL queries alone do not prove a meaningful speedup.

## Stage 7: investigate skinning cost; propose a separate patch

Inspect `scene/SubMeshEntity.cpp`, `scene/MeshEntity.cpp`,
`impl/VertexBufferVBO.cpp`, mesh caches, and CPU geometry consumers.

Measure representative animated enemies and ragdolls: CPU skinning time,
PackVertices time, upload bytes/calls, and frame impact. Compare a scene with
few animated meshes and a deliberately animation-heavy case. Use available
profiling tools; do not introduce a permanent benchmark framework for this.

If material, propose the smallest justified change: avoiding updates when the
pose is unchanged, separating static/dynamic attributes, or GPU skinning. Explain
why it wins over the alternatives. A GPU implementation needs a GL 3.3-compatible
bone palette, weight/index attributes, shared deformation in ambient/light/fog/
shadow/refraction passes, and an explicit policy for CPU bounds, physics,
queries, and caches. Retain unweighted bind-pose fallback, four-influence bounds,
tangent handedness, ragdoll poses, and offscreen shadow-caster updates.

Do not ship partial GPU skinning that renders the camera correctly but leaves
shadows or CPU geometry wrong. If profiling is unavailable, report the missing
measurement and leave this as a follow-up; do not estimate savings as fact.

## Validation and handoff

At each stage, build and run the affected existing regression suites. Add tests
for the behavioral risks above; dead-code removal needs compilation and caller
searches rather than implementation-mirroring tests. Run ASan/UBSan for changes
to dynamic light/queue storage and object lifetimes when the preset is available.

At final validation, rebuild and run both CTest suites. Exercise the EGL storage
suite under available GL 3.3-with-storage-extensions and GL 4.3 contexts as well
as the normal driver. An unavailable context or skipped CTest is a limitation,
not a passing graphics check. Build Windows when its toolchain is available.

Use fresh same-environment captures and unchanged `tools/refshots.py` comparison
tolerances. Capture default, motion-blur, scaled/FSAA, and effects-lab sets;
retain all UI coverage and effects pixel/live checks. Separate intentional stage
1 ordering changes and added >30-light contributions from structural changes.
Reproduce apparent noise with an unchanged baseline before attributing it to
known variation. Do not reuse historical `/tmp/step*` captures without verifying
their revision, settings, and existence.

Update README/CHANGES/ROADMAP only with what is actually implemented and tested,
preserving the user's existing edits. Append completion status and concrete
validation evidence to this plan. Do not mark missing visual/platform checks
complete. The worker's final handoff should include:

- A per-stage description of changes and any departures from this plan.
- Exact build/test/capture commands, results, skips, and artifact/log locations.
- Intentional visual changes and representative before/after/diff images.
- Evidence for reduced resolves/state queries and any measured performance gain.
- Remaining limitations, the stage 7 recommendation, and areas needing review.

Leave the final diff available for independent review; passing CTest alone is
not evidence that transparent ordering, refraction, or cache restoration is
visually correct.

## Implementation status (worker handoff, 2026-10-17)

The worker implemented the submission, storage and state changes for stages
0–6. The final review below finishes the stage 4 execution refactor and records
the remaining validation limits. Stage 7 is an inspection with no profiling
data available.

### Per-stage changes

- **Stage 0 (baseline).** Starting revision `c2ea1a1`; the pre-existing
  working-tree changes to README, CHANGES, CMake, game initialization,
  launch/build/capture scripts, the Windows manifest rename, and the tools
  were preserved. Baseline build and both CTest suites passed before edits.
- **Stage 1 (transparent order).** The comparator now compares A against B.
  Transparent records are appended in `Add()` submission order before any
  pointer-ordered set traversal and stable-sorted in `Compile()` by depth-test
  group then ascending camera-space Z. Equal depths retain submission order.
  `TestTransparentOrder` covers mixed depth flags, distinct/equal depths,
  allocation order different from submission order, duplicate adds, and
  clear/rebuild.
- **Stage 2 (dead interfaces).** Removed `GetTextureBlend()` (base, eight
  overrides and definitions), `mTransQueue`, `eRenderListDrawType_Trans`, the
  object-type queue argument, the unused `abUseDepth` argument, the
  `iRenderState::mfZ` field, and the empty depth-state application/key part.
  Key boundaries and queue tests were updated to the remaining state order.
- **Stage 3 (lights).** `MAX_NUM_OF_LIGHTS` and the fixed per-light arrays are
  gone. `cRenderList` keeps a grow-only `std::vector<cRenderLight>` with the
  light pointer, queue and affected-object count. `Compile()` and
  `RenderLight()` consume the same indexed list, preserving the previous
  pointer-set iteration order. `TestLightList` covers 0, 1, 30, 31 and 64
  lights, an unaffected second object, sector restriction, queue-to-light
  association and 64 → 1 → 64 reuse.
- **Stage 4 (transparent records).** `cTransparentDraw` captures object,
  depth/group, material alpha/blend/program/setup, texture bindings, vertex
  buffer, copied model/inverse matrices, and refraction inputs (including the
  material-selected refraction diffuse texture). `RenderTrans()` executes the
  ordered records. `GetTransDraw()` and `SetupTransDraw()` still run at draw
  time, so stateful custom draws and following-draw state are unchanged.
  Per-object refraction copies, masks, special specular programs and ordering
  remain in the execution path.
- **Stage 5 (scene depth).** `cGLRenderTargets` tracks
  `mbSceneDepthResolved` separately from colour. `BeginScene`, framebuffer
  recreation and completed depth-writing draws invalidate it; clears use the
  same boundary. Repeated `ResolveSceneDepth()` is free until a real write.
  `ResolveScene()` omits the depth blit when the copy is current. After the
  colour resolve, depth writes go to the resolve attachment and an older
  multisampled copy is never blitted over them. EGL tests count real depth
  blits and cover repeated reads, write-mask-only changes, prepare-only,
  shadow/resource-target/colour-only work, new frames, FSAA on/off and the
  post-resolve case.
- **Stage 6 (fullscreen state ownership).** `DrawFullscreenPass()` now takes
  logical program/material UBO state from the program owner, texture/sampler
  bindings (2D and array separately) and masks/capabilities from
  `cGLGraphicsState`, and viewport from `cGLRenderTargets`. Program application
  tracks the actual GL binding separately from the logically selected program
  used by `PrepareDraw()`. Reloads update the saved current variant handle.
  Texture deletion and storage replacement match the cached handles as well
  as the selected resource, so an earlier `SetTexture(nullptr)` cannot leave a
  stale name for a later pass. `EndScene()` and `PresentFrame()` disable and
  restore scissor through the state owner, so the first fullscreen pass cannot
  re-enable it over the SMAA clears. Raw external GL users call
  `ResyncExternalState()`. The six clip-distance queries were removed after
  confirming nothing enables them. The EGL hostile-state tests were extended
  for array restoration, unbind-then-delete, unbound resize, scissored SMAA
  clears, default-program selection and reload rebinding; existing restoration
  assertions were not weakened.

### Material departures and review fixes

- Transparent records capture material state in `Add()` rather than in the
  draw loop. Object callbacks are still queried at draw time. Model matrices
  are copied after `GetInvModelMatrix()` to reproduce the old scratch-matrix
  value rather than silently changing particle drawing.
- `iMaterial` tolerates a null renderer for context-free material fixtures;
  `cGraphics` now initializes `mpRenderList` (previously uninitialized when no
  `Init()` ran).
- Five review findings were addressed: logical vs actual shader binding,
  water's material-selected refraction texture, deleted-texture restoration,
  reload-stale current handles, and never overwriting newer resolved depth.
  Two follow-ups then made handle cleanup independent of the selected resource
  pointer (including unbound replacement) and moved the internal scissor
  changes in `EndScene()`/`PresentFrame()` onto the state owner. The new SMAA
  test was verified to fail with the old raw scissor disable before restoring
  the owner-based fix.

### Validation performed

```
cmake --build --preset default -j 8
ctest --test-dir build --output-on-failure          # 2/2 passed
cmake --build --preset asan -j 8
ctest --test-dir build-asan --output-on-failure     # 2/2 passed, ASan/UBSan
MESA_GL_VERSION_OVERRIDE=3.3 MESA_GLSL_VERSION_OVERRIDE=330 ./build/renderer_storage
MESA_GL_VERSION_OVERRIDE=4.3 MESA_GLSL_VERSION_OVERRIDE=430 ./build/renderer_storage
```

The EGL suite passed on Mesa llvmpipe reporting GL 3.3, GL 4.3 and the
driver's normal GL 4.5 core context. Logs: `/tmp/openhpl1-cleanup-*.log`.

Fullscreen passes now issue one `glIsEnabled(GL_STENCIL_TEST)` query per pass
plus the explicit external resync queries in the fixture. Program, material
UBO, texture/sampler, viewport, mask and capability restoration no longer
queries GL. This is a query-count reduction, not a measured frame-time gain.

### Limitations

- No fresh visual captures were produced: the Penumbra game data is absent
  (`~/.local/share/Steam/steamapps/common/Penumbra Overture`) and `gamescope`
  is not installed. `tools/refshots.py capture` stops at the game-data check.
  The stage 1/3 picture changes (ordering and >30 lights) and stage 4/6 visual
  preservation therefore remain **unvalidated**, not passed.
- No Windows toolchain was available; the Windows build was not run.
- There is no measurement of GPU skinning cost (stage 7). Stage 7 findings are
  static: per animated submesh, every rendered frame recomputes bone matrices,
  rewrites up to four influences per vertex for position/normal/tangent, calls
  `PackVertices()` for all interleaved attributes, and reallocates with
  `glBufferData`. The smallest candidate is to skip skinning when the pose is
  unchanged and upload dynamic ranges with `glBufferSubData`; a GPU path would
  need a GL 3.3 bone palette, weight/index attributes, shared deformation in
  all passes, and an explicit CPU-bounds/physics/cache policy. No savings are
  claimed without profiling.
- `git diff --check` reports trailing whitespace in the pre-existing Windows
  launcher edits; those unrelated changes were left untouched.

## Final review and completion

The two follow-up fixes for unbound texture deletion/replacement and internal
scissor changes were verified in the current tree. The remaining work was
completed as follows:

- `RenderTrans()` now resolves each custom draw once, executes that object's
  refraction, applies shared `iRenderState` setters, and draws in order. The
  former duplicated alpha/blend/texture/buffer/matrix state implementation is
  removed. Refraction copies and destination-alpha masks have separate helpers.
  A change of setup object with the same program reruns setup and matrix
  callbacks. Dead texture-blend arrays and the unused trailing key slot are gone.
- A real EGL renderer fixture exercises ordinary transparency → custom draw →
  masked refraction → ordinary transparency. It checks the resulting pixel
  against the expected blend/copy sequence, checks callback count/timing and
  custom depth/culling restoration, and covers setup-only transitions.
- Storage replacement now preserves a different array selected on a unit when
  a lingering 2D resource becomes an animation array. An EGL regression checks
  both cached and actual GL bindings.
- `ResyncExternalState()` is documented as resynchronizing capabilities and
  masks; raw clients remain responsible for restoring bindings and functions.

Final checks:

```sh
cmake --build --preset default -j 8
ctest --test-dir build --output-on-failure
cmake --build --preset asan -j 8 --target engine_regressions renderer_storage
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-asan --output-on-failure
MESA_GL_VERSION_OVERRIDE=3.3 MESA_GLSL_VERSION_OVERRIDE=330 ./build/renderer_storage
MESA_GL_VERSION_OVERRIDE=4.3 MESA_GLSL_VERSION_OVERRIDE=430 ./build/renderer_storage
```

Default and ASan/UBSan suites pass, and both restricted-GL runs execute and
pass. Leak detection is disabled for final sanitizer checks because
LeakSanitizer itself fails under the review environment's ptrace restriction;
this does not establish a leak-free run. Sanitizer build output is at
`/tmp/openhpl1-renderer-final-asan-build.log`; GL runs use
`/tmp/openhpl1-renderer-final-gl33` and `/tmp/openhpl1-renderer-final-gl43`.

Full game reference captures and Windows validation remain unavailable. The
small EGL scene verifies execution behavior without replacing those checks.
GPU skinning and frame-time performance measurements remain follow-up work.
The renderer commit excludes the pre-existing executable rename and launcher,
manifest, CMake, initialization and tool changes; those remain in the worktree.
