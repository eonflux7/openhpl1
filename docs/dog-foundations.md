# Dog AI and animation foundations

The `dog-foundations` branch improves the original Dog in code. It uses the
installed game's mesh, textures, skeleton, animation clips, animation events,
and entity definitions. No comparison lab, replacement assets, or animation
packs are included.

## Behaviour

- A Dog stops refreshing its remembered player position after a failed sight
  check. Attention, hidden pursuit, backing away, and attack aiming use that
  observation. An attack can still hit a player who enters its collision
  volume; damage uses the original range, shape, and timing.
- An audible sound during an unseen hunt supplies its actual source position.
  The Dog approaches the observation, then pauses and turns using the original
  Idle/Walk clips. Search uses the existing `HuntForLostPlayerTime` budget
  (six seconds for the standard Dog); fresh sight or sound resumes pursuit.
- Direct pursuit checks character clearance against walls, doors, and heavy
  props. Small pushable props remain traversable. After getting stuck, a route
  retry checks the graph against current obstacles. A failed route stops the
  Dog; a visible unreachable player is watched while the route retry waits.
- Near a clear path corner, the Dog aims a short distance into the following
  segment. Clearance checks prevent the smoother steering from cutting
  through a wall. Original node arrival, speeds, acceleration, and normal turn
  rates remain in use; a turn cannot overshoot its target within one tick.
- Dog states return after handing control to another state, so an old Hunt,
  Patrol, Investigate, MoveTo, or Flee tick cannot undo the transition.
  Backward movement no longer counts as stuck solely because its speed is
  negative. Rear-wall correction runs per Dog and requires an actual hit.

The pursuit thresholds now load `HuntMinSeeChance` / `HuntMinHearVolume`
instead of the idle attributes. For the standard original definition this
means **0.2 instead of 0.3** during hunting. Missing attributes retain the idle
fallback. This restores authored settings and may improve reacquisition.
Sight range, field of view, difficulty multipliers, and the original automatic
close-contact detection rule have not been retuned.

## Animation

Gait selection and playback use horizontal travel, so falling and climbing a
step no longer speed up the stride. Existing walk/run phase synchronisation
and authored base playback speeds remain intact. Reusing a clip updates its
loop, speed, and movement-override options without restarting its phase;
fixed playback recovers from a previous zero movement speed. An animation
completion callback fires once per playback and cannot apply old settings to
a replacement clip.

The door-hit countdown now advances once per tick, matching the authored
`BreakDoorDamageTime` (0.7 seconds for the standard Dog), instead of effectively
halving it. Jump attack timing and damage values remain unchanged.

These shared playback, perception, state-lookup, turning, and stuck-counter
fixes also benefit other enemy types. Last-observation pursuit, the new search,
corner lookahead, and failed-route stopping are enabled for Dogs.

This pass does not add foot IK, head tracking, body bend, breathing poses, jaw
animation, attack anticipation, or a rewritten ragdoll/get-up blend. Those
need separate visual work and playtesting. Reducing incorrect playback speed
helps sliding, but is not a full paw-contact solution.

## Verification

Build with `BUILD_TESTING=ON` (CTest's default), then run:

```sh
cmake --build --preset default
ctest --test-dir build --output-on-failure
python3 tools/check_dog_foundations.py --build build

cmake --build --preset asan
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-asan --output-on-failure
python3 tools/check_dog_foundations.py --build build-asan --maps all --timeout 600
```

The Python runner needs gamescope and the installed game data; `--game-dir`
can override its Steam location. It runs silently in a headless session with
an isolated profile, copies the maps to keep legacy AI cache writes out of
the installation, and redirects mesh/animation caches. `HOME` is unchanged.
Output, logs, and test saves live under the build's `dog-foundations-check`
directory unless `--out` is supplied.

The opt-in in-game regressions load the original Dog and exercise actual
states, clips, clearance rays, and graph edges. They cover remembered aim,
sound reacquisition, search duration, visible blocked targets, open/blocked
corners, backwards movement, playback reuse, gait phase, vertical movement,
door impacts, and completion delivery. The temporary Dog and its runtime
navigation fixture are removed before the ordinary test-map save/reload;
original-level smoke checks cover enemy save/reload on authored graphs.

Leak detection is disabled for graphics-driver teardown. The runner suppresses
only AngelScript's existing `nonnull-attribute` warning in `as_string.cpp`
from zero-length `memcpy` during script compilation; other UBSan findings stop
the run. Suppressions and logs are kept in the output directory.

Manual checks still needed: hide after turning a corner, lure with sound from
another position, block a route with a heavy prop, approach a closed door,
compare pursuit on each difficulty, and observe attack/get-up transitions.
Automated smoke checks establish stability, not final encounter balance or
animation quality.
