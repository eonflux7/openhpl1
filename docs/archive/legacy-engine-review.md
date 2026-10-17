# Legacy engine review — 2026-10-02

> Archived on 2026-10-18. This is a historical snapshot, not current
> status or agent instructions. See the [current roadmap](../../ROADMAP.md)
> and [archive index](README.md) for context. Paths under old build
> directories and `/tmp` may no longer exist.

Reviewed against `0d37bfe`, using the original import (`cc845cb`) to identify
logic that has survived the port. “Untouched” here includes files with
mechanical nullptr/override/include changes, and original functions inside
partially modernized files. The current AO and renderer working-tree changes
were outside this review. No implementation files were changed.

This was a targeted skim of AI, skeletal animation, math, audio, resources,
serialization and physics, with caller tracing and small reproductions. It
was not an exhaustive engine audit or a playthrough. The findings below are
pre-existing logic defects; whether the shipped assets trigger each edge
case needs a separate asset audit.

These are the findings I would address first:

1. **Bone-weight compilation reads outside its allocation and does not
   normalize the weights. High priority; reproduced.**

   [SubMesh.cpp](../../engine/sources/graphics/SubMesh.cpp), lines 173–195.
   The loop tests `pWeight[lNum]` before checking the index, and allows
   `lNum <= 4` even though each vertex has four slots, indexed 0–3. A vertex
   with four nonzero influences reads the following vertex's weights, or
   beyond the allocation for the final vertex.

   The normalization loop then divides `pWeight[lNum]`, repeatedly, instead
   of `pWeight[i]`. For fewer than four influences it modifies the zero
   sentinel and leaves the actual weights unchanged. For four influences
   it can also write outside the vertex's slots. Discarding excess bone
   influences during import makes actual normalization particularly useful.

   Calling the actual `CompileBonePairs()` on a one-vertex buffer with four
   influences produced an ASan heap-buffer-overflow at line 187. With two
   equal weights of 0.2, the resulting weights were still 0.2 and 0.2,
   instead of 0.5 and 0.5.

   Fix the bound to `lNum < 4`, evaluate it before dereferencing, divide
   `pWeight[i]`, and validate vertex/bone indices and the total weight.
   Check enemy animations and ragdolls after changing normalization.

2. **Collada cache numeric parsers can overrun both input and stack buffers.
   High priority; reproduced.**

   [String.cpp](../../engine/sources/system/String.cpp), lines 653–722;
   [MeshLoaderColladaLoader.cpp](../../engine/sources/impl/MeshLoaderColladaLoader.cpp),
   for example lines 1004, 1017 and 1224.

   `FloatStringToArray()` and `UIntStringToArray()` keep reading until they
   have produced the requested number of values. They do not stop at the
   input terminator when the declared count is too large. Their token buffer
   has 20 bytes with no length check, and only a literal space separates
   values. A long decimal overruns the token buffer; truncated data or
   newline/tab-separated values can make them read beyond the string.

   ASan reproduced input overruns in both parsers with `"1 2"` and an
   expected count of three, a stack write overflow with
   `"0.12345678901234567890123456789"`, and an input overrun with
   `"1\n2\t3"` and an expected count of three.

   These functions are used by the generated Collada XML cache loader;
   this is not the ordinary raw `.dae` tokenization path. Internally
   generated caches normally use spaces, which limits the whitespace
   trigger, but corrupt or edited caches still exercise the unsafe parser.
   Parse directly from bounded input, report conversion/count failures,
   and let an invalid cache fall back to the source asset. Also validate
   declared vector sizes against the number of XML children.

3. **Skinning loses its vertex position after an unweighted vertex.
   Medium priority; confirmed by source and caller tracing.**

   [SubMeshEntity.cpp](../../engine/sources/scene/SubMeshEntity.cpp), lines
   209–262. `if (*pWeight == 0) continue;` advances the loop's vertex index
   without advancing the position, normal or tangent pointers. The next
   weighted vertex therefore uses the preceding vertex's geometry and
   writes into the preceding output slot. Later vertices remain displaced
   in the arrays. The importer explicitly allows unconnected vertices and
   emits a warning for them in `CompileBonePairs()`.

   Address each vertex by its index, or always advance the array pointers.
   Give an unweighted vertex an explicit bind-pose fallback. The influence
   loop at line 238 also reads `*pWeight` before checking `lCount < 4`, so
   a final vertex with four influences has a second out-of-bounds read here.
   No live-game deformation was reproduced in this review.

4. **A* discards cheaper routes to an already-open node.
   Medium priority; reproduced.**

   [AStar.cpp](../../engine/sources/ai/AStar.cpp), lines 259–282;
   [CharacterMove.cpp](../../game/enemies/CharacterMove.cpp), line 372.

   When insertion into the open set fails because the same AI node is
   already present, `AddOpenNode()` deletes the candidate and returns. It
   never compares distances or updates the existing node's parent. That
   breaks the relaxation step required to find the shortest route and can
   give enemies unnecessary detours.

   An isolated call to the actual method inserted a node at distance 10,
   then rediscovered it at distance 2; the stored distance remained 10.
   Update an open node when a better route is found, then update its queue
   priority. Define the accumulated cost carefully: the current `Cost()`
   multiplies the entire traveled distance by the last edge's height factor.
   A correction should be checked on dog/spider navigation, because it
   intentionally changes path selection.

5. **Failed character conversion is treated as a huge string size.
   Medium priority; reproduced.**

   [String.cpp](../../engine/sources/system/String.cpp), lines 33–50;
   [posix.cpp](../../engine/sources/impl/platform/posix.cpp), line 89.

   `mbstowcs()` and `wcstombs()` return `size_t(-1)` on an invalid or
   unrepresentable sequence. Both conversion helpers pass that value
   straight to `resize()`. A filename containing invalid bytes for the
   active locale can therefore throw during resource-directory scanning.
   A valid non-ASCII filename can also trigger it under the C locale.

   `To16Char("\xc3\xa9")` under `LC_CTYPE=C` reproduced a string exception.
   Check both conversion calls' results and define how invalid names are
   reported or skipped. Prefer an explicit encoding policy for UI text;
   filesystem names also need a deliberate policy rather than an unchecked
   locale conversion.

6. **Degenerate UV triangles produce NaN tangents.
   Medium priority; reproduced.**

   [Math.cpp](../../engine/sources/math/Math.cpp), line 1304;
   [VertexBufferVBO.cpp](../../engine/sources/impl/VertexBufferVBO.cpp), line 163;
   [MeshLoaderColladaHelpers.cpp](../../engine/sources/impl/MeshLoaderColladaHelpers.cpp),
   line 1569.

   Tangent generation divides by the UV determinant without checking zero
   or near-zero values. A triangle whose three UVs are identical produced
   `(-nan, -nan, -nan, 1)` in the actual generator. These values can spread
   through shared-vertex accumulation into other triangles' tangents and
   normal-map shading.

   Skip degenerate UV contributions, and choose a finite tangent orthogonal
   to the normal when a vertex has no usable tangent. Check nearly
   degenerate cases as well as an exactly zero determinant.

7. **3D sounds use inconsistent global-volume masks.
   Medium priority; confirmed by source.**

   [SoundHandler.cpp](../../engine/sources/sound/SoundHandler.cpp), lines
   720, 791 and 901.

   The relative 2D path correctly tests a sound's `mEffectType` against
   `mAffectedByVolume`. The relative 3D path tests its list's `aTypes`
   against `mAffectedByVolume`; the positioned 3D path tests `aTypes`
   against `mEffectType`, omitting `mAffectedByVolume` entirely. The list
   containing a sound and its chosen effect category are separate arguments
   in `Play()`.

   Thus `SetVolume(..., eSoundDest_Gui)` can still affect ordinary positioned
   world sounds, and cross-category sounds can react differently depending
   on their spatial mode. Use the entry's effect category against the global
   affected mask consistently. Compare GUI, relative 3D and positioned 3D
   sounds under a selective fade. This was not reproduced with live audio.

8. **The sound channel ID counter starts uninitialized.
   Small fix; confirmed by source.**

   [SoundHandler.h](../../engine/include/sound/SoundHandler.h), line 202;
   [SoundHandler.cpp](../../engine/sources/sound/SoundHandler.cpp), constructor
   at line 42 and the `SetId(mlIdCount)` calls in both playback paths.

   The constructor initializes `mlCount`, but never `mlIdCount`. Both
   playback paths read it in `SetId()` and increment it. These IDs are used
   by `IsValidId()` to distinguish an old channel from a new one allocated
   at the same address. Initialize the counter and define wraparound;
   an unsigned counter avoids eventual signed-overflow undefined behavior.

There are also several smaller robustness and behavior issues:

- **AI grids with one node, or nodes aligned on a single axis:**
  `AINodeContainer.cpp:626` divides by a zero grid dimension and casts
  infinity/NaN to `int`. A one-node container reproduced two
  `float-cast-overflow` diagnostics. Node insertion already guards those
  divisions; querying needs the same guard. `BuildNodeGridMap()` also
  assumes at least one node, and cached node/edge names need validation.
- **PID history size:** `PidController.h:76` accepts zero or negative sizes
  and does not reset its ring indices when resized. Zero history reproduced
  a null write in `Output()`; shrinking a used history from 10 to 2
  reproduced a vector bounds assertion. `EntityLoader_Object.cpp:929`
  accepts the history size from XML. Require a positive size and reset or
  adjust the indices when changing it. A running integral can later remove
  the per-update history sum.
- **Music loop changes:** `MusicHandler.cpp:130` returns early for the same
  song at the same target volume, ignoring a changed `abLoop`. When volume
  does change, it updates the entry's loop flag without updating the live
  stream. This can also make saved music state disagree with playback.
  Apply loop changes to the channel before deciding there is nothing to do.
- **Animation events across the loop boundary:** `MeshEntity.cpp:841` only
  handles `previousTime <= eventTime < currentTime`. When playback wraps,
  previous time exceeds current time and events in that crossed interval
  are missed. Split the interval at the animation's end; account for
  reverse playback if supported. The game can attach sounds to these events.
- **Generic engine animation serialization:** `AnimationState.cpp:320`
  serializes `mfDefaultSpeed`, whereas save/load copy `mfBaseSpeed`, which
  is absent from the registered fields. This is a real schema mismatch,
  but Overture's entity saves use their own animation save structure, so
  do not assume this explains a current game save bug. Repair the generic
  path if it is retained, with explicit handling of existing saves.

The most promising optimization candidates are these. Complexity and
allocation patterns were inspected; no CPU/GPU speedups were measured.

| Area | Current work | Useful change | Practical limit |
| --- | --- | --- | --- |
| A* search (`AStar.cpp:264`, `:287`) | Allocates a search node before checking whether it is already visited; a pointer-ordered set requires scanning all open nodes to find the cheapest | One reusable record per AI node and a cost-ordered heap, alongside correct relaxation | Measure search time and node counts on actual enemy maps; path changes need play-testing |
| Tangent generation (`Math.cpp:1331`) | Compares every pair of vertices: V(V−1)/2 position/normal checks | Group equal position/normal vertices first, then compare only within those groups | Primarily loading/cache-generation time; preserve accumulation order if original shading must match |
| Sound validity (`SoundHandler.cpp:445`, `:469`; `SoundEntity.cpp:353`) | Each sound entity repeatedly scans the channel lists to validate its channels; total work can grow quadratically with sound count | Maintain an index of live channels and IDs, synchronized on insertion/removal | Keep the existing protection against stale pointers and address reuse |
| Animation sampling (`AnimationTrack.cpp:182`) | Starts a linear keyframe search at frame zero on every sample | Binary search, or a per-instance cursor with reset on looping/seeking | Tracks can be shared across instances; do not put a single playback cursor in the shared track |
| CPU skinning (`SubMeshEntity.cpp:193`) | Transforms positions/normals/tangents and uploads dynamic vertex data whenever a visible animated submesh is updated | First cache results by pose generation; consider GPU skinning if profiling warrants it | The existing sleep shortcut covers sleeping ragdolls, not all unchanged poses; stencil shadows also consume CPU positions |

I would fix parser and bone-weight memory errors first, then skinning and
pathfinding correctness. Sound-counter initialization is cheap enough to do
alongside those fixes. For performance, measure A*, sound validation and
asset loading before committing to GPU skinning or wider physics changes.
The character collision path already queries the portal container, so it
should not be described as a naive scan of every world body.

Validation used temporary C++ harnesses compiling the actual engine source
files with ASan/UBSan, an additional float-cast-overflow check, and standard
library bounds assertions. Virtual-pointer sanitization was excluded from
the isolated builds to avoid linking unrelated engine RTTI. Harnesses and
logs are under `/tmp/overture-engine-skim*` and `/tmp/overture-engine-bones*`.
The bone harness used a minimal one-vertex buffer and a logging stub, without
replacing the function under review. The A* reproduction exposed its internal
method for direct testing; it was not a complete navigation simulation.

Targeted clang-analyzer checks through clang-tidy completed on `AStar.cpp`,
`AINodeContainer.cpp`, `SubMeshEntity.cpp`, `SoundHandler.cpp`,
`MusicHandler.cpp` and `String.cpp` without warnings. That result does not
cover the whole repository, and the targeted reproductions demonstrate why
these paths need behavioral tests beyond static analysis. No game smoke test
or rendering comparison was run, since no implementation was changed.
