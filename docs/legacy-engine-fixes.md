# Legacy engine fixes

Implemented on `engine-fixes`, based on `0d37bfe`, following the legacy-engine
review. The changes cover AI, skeletal animation, asset parsing, math and audio.

- Bone influences are validated, bounded to four, and normalized correctly.
  Recompilation releases the old arrays, and weights remain packed even if a
  tiny normalized influence underflows. CPU skinning keeps unweighted vertices
  in their bind pose and checks the influence bound before reading a weight.
- Numeric array parsing reports malformed, truncated, extra and non-finite
  values, accepts ordinary whitespace, and avoids fixed token buffers.
  Collada cache loading checks required elements/attributes, sizes, child
  counts, geometry indices and joint/extra-vertex bounds. Failed loads discard
  partial results and fall back to the source DAE, replacing the bad cache.
- A* relaxes cheaper routes and can reopen a visited node. Height penalties
  accumulate per edge. One owned record per visited node replaces allocation
  on every attempted edge; a cost-ordered open set replaces the linear scan.
  AI grids handle empty and collapsed dimensions and repeated builds. Invalid
  AI caches preserve the current graph and cause the world loader to rebuild
  connections; successful loads replace edges instead of appending duplicates.
- Character conversion checks failure and retains the active-locale policy.
  Unrepresentable input returns an empty string. POSIX directory scans skip
  such filenames, accept long paths, and handle an empty directory argument.
- Tangent generation skips degenerate UV contributions and provides a finite,
  unit fallback orthogonal to the normal. Matching position/normal vertices
  are grouped before smoothing, preserving the original pair order within
  each group. Ordinary keyframe sampling uses binary search; empty tracks
  return an identity transform.
- Relative and positioned 3D sounds use the sound's effect category against
  the affected-volume mask. Channel IDs start at zero and wrap from `INT_MAX`
  to zero without signed overflow, retaining the existing integer API.
  Same-song music requests update the live stream's looping flag even when
  target volume is unchanged; new music channels also receive that flag.
- PID controllers have a valid default history and initialized coefficients.
  Non-positive history sizes become one, resizing resets ring indices, and a
  non-positive timestep returns the proportional term without dividing by it.
- Animation events account for forward/reverse loop crossings and full-loop
  updates. Seeking, paused updates and loading clear the pending interval.
  An update spanning several loops emits each event once. Generic engine saves register
  base speed, retain the old default-speed field, and default missing base speed
  to one. Overture's separate entity-save schema is unchanged.

## Validation

Build and run the headless regression suite with:

```sh
cmake --preset default
cmake --build --preset default
ctest --test-dir build --output-on-failure
cmake --preset asan
cmake --build --preset asan
ctest --test-dir build-asan --output-on-failure
```

The suite calls the actual engine methods. It covers normalization and CPU
skinning, parser failures, directory scanning, A* route relaxation/callbacks/
iteration limits, collapsed grids, AI/Collada cache round trips and rejection,
source fallback, degenerate tangents, animation sampling/events/serialization,
audio volume masks, music loop changes and PID resizing. UBSan halts on errors;
LeakSanitizer is enabled. Both build configurations pass.

The baseline and fixed ASan game builds pass the 26-map smoke test, including
save/reload and the physics test map, with zero reported smoke failures. Tests
use isolated settings/save profiles and OpenAL's null backend. Existing caches
containing NaN or Windows `-1.#IND` tangent data rebuild from source. A second
fixed run loads the regenerated caches without further rejection warnings.

Both game builds report the same pre-existing UBSan non-null-argument warning
at `vendor/angelscript/source/as_string.cpp:187`; game smoke runs log this
warning rather than halt on it. No AddressSanitizer errors occur. Focused
clang-analyzer checks on the changed search, skinning, parsing, animation and
audio implementation files report no warnings.

A temporary `-O2` comparison compiles the original and grouped tangent routines
against synthetic triangle meshes with repeated position/normal groups. Outputs
are bit-identical for those ordinary meshes. One run measured:

| Vertices | Original | Grouped |
| ---: | ---: | ---: |
| 3,000 | 2.52 ms | 0.29 ms |
| 6,000 | 9.90 ms | 0.52 ms |
| 12,000 | 39.46 ms | 1.25 ms |

These are synthetic asset-processing timings, not a game FPS measurement.
Grouping still has quadratic work inside a group of identical vertices.
Navigation and normalized skinning intentionally change behavior; smoke tests
and graph tests do not replace a full enemy chase or ragdoll playthrough.
Live-channel indexing, pose caching and GPU skinning remain profiling candidates.
