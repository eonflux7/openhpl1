# Dog AI and animation foundations

OpenHPL1 improves the original Dog in code. It uses the
installed game's mesh, textures, skeleton, animation clips, animation events,
and entity definitions. The generated test arena uses those original assets;
no replacement assets or animation packs are included.

## AI choice and test arena

**Options → AI → Dog behaviour** offers mutually exclusive Original and
Improved choices. The choice is saved as `AI/ImprovedDogAI` in settings;
Improved is the default. Existing Dogs adopt the choice when
gameplay resumes. A running hunt/search restarts, while an attack or recovery
finishes normally.

Original retains the original pursuit/search decisions: live-player aiming,
the short post-sight-loss position refresh, blind forward search, legacy
blocked-route fallback, nearby-node sound targeting, and idle thresholds
during hunting. Improved enables the observation, search and navigation
changes below, including the authored hunt thresholds. Both modes retain
shared animation and stability fixes, including corrected door-impact timing
and blocked-attack checks;
Original is a comparison of decision behaviour, not a bit-for-bit legacy build.
This setting currently applies to Dogs only.

Launch the generated arena with:

```sh
cmake --build build
OPENHPL_MAP=dogtest ./run.sh
# Or start in a particular bay:
OPENHPL_MAP=dogtest:sound ./run.sh
```

The F2 panel opens on entry. Choose a scenario, select an AI mode, and use
**Reset and play** for fresh starting conditions. Reset recreates the original
Dog and that bay's props, restores player health and position, and closes the
panel. F2 reopens the controls and live state/clip/last-observation diagnostics.
The map is also available in the usual F2 map list.

| Bay / start | Test |
|---|---|
| `patrol` | Four waypoints, pauses and corners; disable sight/hearing to observe gait without a chase |
| `hide` | A clear sight lane, central cover and a second corner for hiding and last-position search |
| `sound` | Occluding cover and two real, hearable original wood-impact sound lures |
| `props` | Original heavy and small pushable crates, a narrow route and a wide detour |
| `door` | Original locked mine-shaft door with authored health, toughness and debris; reveal your position once to seed a pursuit |
| `attack` | Standing/crouched jump attacks, an optional low step, and real ragdoll/getting-up via Knock down |

**Reveal player position once** deliberately supplies one known position for
fixtures where a wall or door prevents initial sight. Sound lures go through
the normal sound-trigger system. God mode defaults on within the arena and
can be disabled; the previous value is restored when leaving. Disabling
perception stops fresh sightings and hearing, but does not cancel a hunt
already in progress: reset to start a clean patrol. Loading an arena save
starts a fresh fixture at the nearest bay rather than resuming its encounter.

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

Both Dog AI modes check for a clear line to the player before lunging. This
lets the existing stuck/door-breaking logic run when the player stands just
behind a closed door. Shared shape attacks check occlusion before damaging
any bodies, supply their own skipped body, and block player damage, knockback
and hit feedback together. A door also shields the player from the strike
that breaks it; subsequent attacks through the open doorway work normally.

These shared playback, perception, attack, state-lookup, turning, and stuck-counter
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
python3 tools/check_dog_foundations.py --build build --maps 'dogtest testmap'
python3 tools/check_dog_foundations.py --build build --maps dogtest --ai original

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
The arena checks all six stations under both AI modes: original entities and
clips, sight-memory policy, thresholds, patrol reachability, cover and crate
detours, sound-source positions, doors, knockdown/reset and switching an
already spawned Dog. The arena's authored graph also survives smoke save/reload.
Door tests run real physics, animation and AI ticks in both modes, with the
player nearby and farther away. They require the Dog to enter BreakDoor,
damage the authored door and create its original debris without hurting the
player behind it. Separate strikes verify closed-door and solid-wall
protection, stale skipped-body state, the breaking strike, and an unobstructed
bite's damage and knockback.

Leak detection is disabled for graphics-driver teardown. The runner suppresses
only AngelScript's existing `nonnull-attribute` warning in `as_string.cpp`
from zero-length `memcpy` during script compilation; other UBSan findings stop
the run. Suppressions and logs are kept in the output directory.

Manual checks still needed: hide after turning a corner, lure with sound from
another position, block a route with a heavy prop, approach a closed door,
compare pursuit on each difficulty, and observe attack/get-up transitions.
Automated smoke checks establish stability, not final encounter balance or
animation quality.
