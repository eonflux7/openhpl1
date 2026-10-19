# OpenHPL1 roadmap

One plan for the shared HPL1 engine and Penumbra: Overture, Black Plague and
Requiem. Reconciled on 2026-10-18 against the working tree, including the
renderer and project-name changes already present there.

Overture is the main supported game. Black Plague and Requiem are experimental:
their maps load and much of their game code is implemented, but neither has a
verified full playthrough. Game assets come from the user's own installations.

This file tracks current work. [README.md](README.md) covers building, running
and test tools; [CHANGES.md](CHANGES.md) records implementation history.
[Technical notes](docs/README.md) describe current subsystems. The previous
[engine roadmap](docs/archive/ROADMAP.md) and
[porting roadmap](docs/archive/PORTING-ROADMAP.md) retain detailed historical
checklists. Their old settings and instructions are not current requirements.

## Status and priorities

| Area | Implemented | Still needed |
| --- | --- | --- |
| Native build (Milestone 0) | C++17, 64-bit Linux and Windows, SDL3, GLSL, Newton 2.36, AngelScript 2.38; MSVC build/smoke pass; captured Windows startup crashes traced to EFX slot mutex lifetime and fixed | Broader platform and audio lifecycle coverage |
| Gameplay parity (Milestone 1) | Map smoke/save/reload tools, physics lab, engine fixes, Dog AI/animation checks | Full playthroughs, original-save compatibility and Newton 1.53 comparisons |
| Widescreen and usability (Milestone 2) | Wide UI, FOV, TrueType fonts, shader gamma, interpolated rendering, display options | Remaining UI/display and frame-pacing work below |
| Renderer foundation (Milestone 3) | Core GL, float scene targets, shader includes/reload, reference captures and GPU profiler | Live display reconfiguration; hardware-driver coverage |
| Lighting and materials (Milestones 4–5) | Linear lighting, shadow maps, bloom, AO, SMAA, improved shading/lights, soft/lit particles, lens and film effects | Remaining visual work and tuning below |
| Renderer cleanup (Milestone 6) | Steps 1–13 implemented; game/UI/effects captures and Windows smoke recorded; boat-cabin difference confirmed as the step-11 ordering correction | Resolve ice-cave capture differences; frame-time measurements and broader cache checks |
| Black Plague / Requiem | Shared executable, scripts, entities, Infected, terminals, swimming, menus, endings and save support | Playthroughs, targeted manual checks and four script stubs |

The next work is to resolve the recorded renderer differences, then close
gameplay and progression gaps across all three games.
Further visual and performance work should follow evidence from those runs.

The renderer now has one improved look. Milestone 6 step 4 removed the original
lighting, stencil shadows, old post effects and their Original/Improved
switches. Quality and taste settings remain. The `original-look` tag preserves
the earlier renderer; do not treat Milestones 4–5's archived dual-path plan as
work to restore. The Dog's Original/Improved AI choice is separate and remains.

The [naming/readability review](docs/archive/naming_cleanup.md) is complete through A7
(2026-10-19), with compatibility decisions and validation limits recorded there.
Outstanding gameplay, renderer, Windows and performance checks are listed below.

## Validation to close first

Updated 2026-10-19: current normal and ASan/UBSan CTest suites pass (leak
detection disabled), as do software EGL checks on GL 3.3 and 4.3. Fresh Windows
captures and MSVC build/smoke results are now recorded in
[CHANGES](CHANGES.md#renderer-steps-1113-game-validation-2026-10-19).
Scene preservation is still open: comparisons below include real mismatches.

- [x] Capture and compare the current renderer against a suitable pre-change
      baseline: default settings, motion blur, Render Scale 1.5 with 4× FSAA,
      all 2D screens and the effects lab. Retain comparison tolerances and
      record intentional ordering and >30-light corrections separately.
      Baseline `c2ea1a1` versus `59f1b97`, both with identical extended UI
      capture code: default 95/98 match, motion blur 93/98, scaled/MSAA 32/34,
      effects 29/29. All 15 UI screens match in each scene pair; effects
      pixel/live checks pass. Each all-map pair has two changed frames under
      the existing unstable-map policy. Tolerances and exceptions are unchanged.
- [x] Resolve boat-cabin view 2 (default/motion blur). Draw logs show the
      depth-disabled `halo1` moves from last to first among the same ten
      transparent objects. Reverting only depth-test grouping in a temporary
      current executable restores all four baseline boat frames in both
      modes, plus all 19 default boat/UI frames. This is the step-11 ordering
      correction; tolerances and unstable-map exceptions are unchanged.
      The mixed-depth regression now puts its depth-disabled object in front
      so that Z-only sorting fails the test.
- [ ] Resolve ice-cave views 1/4 (motion blur/scaled MSAA). A fresh unchanged
      baseline motion capture reproduces the 103/837-pixel differences;
      the current repeat matches its original 98 frames, and the repeated
      cross-build motion pair has no ice-cave mismatch. Both scaled repeats
      match their own builds, while the cross-build pair still differs by
      114/727 pixels. Causes remain open; neither an intentional correction
      nor an additional unstable-map exception is established. See the
      [follow-up evidence](CHANGES.md#boat-cabin-ordering-and-ice-cave-repeats-2026-10-19).
- [x] Run GL debug validation in gameplay and the effects lab, including
      intro, death and credits screens that earlier checks did not cover.
      Completed NVIDIA GL 4.6 captures show only synchronized readback
      performance warnings; the current 28-map smoke log has no GL messages.
- [x] Build and smoke-test current Windows/MSVC sources, including the
      `openhpl1.exe` and manifest rename; earlier step 10 success is historical.
      MSVC 19.51 build passes with existing vcpkg dependencies; isolated
      Overture smoke/save/reload passes 28/28 maps, exit 0. MSVC CTest targets
      are disabled. Successful retries do not close the startup issue below.
- [x] Diagnose the captured Windows startup access violations. Available dumps
      identify invalid EFX slot mutexes in the audio worker, including the
      `ntdll.dll` case. Slot mutex ownership and the atomic worker stop flag
      are fixed. Normal and ASan/UBSan audio regressions pass; a patched MSVC
      build completes 20 consecutive isolated startup/save/reload runs and
      the 25 shipped-map smoke test, exit 0. See [audio lifecycle](docs/audio.md)
      and [validation](CHANGES.md#efx-slot-startup-crashes-2026-10-19).
      This does not close the separate heap-corruption-on-exit report.
- [ ] Check a Mesa hardware driver; software llvmpipe coverage already exists.
      Unavailable here: no `/dev/dri`. Linux game captures also lack gamescope;
      the completed game captures above use Windows/NVIDIA and Steam assets.
- [ ] Measure CPU/GPU frame times for steps 11–13 on representative scenes.
      Program-bind/upload reductions and fewer shadow draws are established
      by fixtures; full-game speedups are not measured.
- [ ] Compare cached and uncached shadows (F4 Shadows budget zero versus the
      default 128 MiB): moving lights/casters, alpha animation, cube-face
      visibility, scene changes and constrained budgets. Include CPU signature
      cost and memory use. See [shadow-cache contracts](docs/shadow-cache.md).

## Gameplay parity across the three games (Milestone 1)

- [ ] Play Overture from start to finish against the Steam build. Test old
      saves by loading them and continuing play, not only round-tripping new
      saves in the smoke test.
- [ ] Compare Newton 2.36 against 1.53 for doors, drawers, ladders, throwing,
      stacking, the boulder, worm chase and later games' puzzle mechanisms.
      Joint-limit bounce, waking bodies after limit edits and expanding world
      bounds have fixes; they still need in-game comparison.
- [ ] Resolve per-body sleep thresholds (Newton 2 exposes world-wide ones):
      emulate them or retune defaults. Review `GetEnabled()`'s sleep-state
      semantics, `SetEnabled(false)`'s hard freeze and bodies waking on load,
      using `OVERTURE_PHYSLOG` and the test map.
- [ ] Investigate the reported intermittent heap corruption on exit; record a
      reproducer or explicit closure rather than assuming later fixes cover it.
- [ ] Verify music stutter, 3D positioning, fades and sector reverb by ear.
      EFX is implemented and enabled by default unless disabled in settings;
      the old roadmap's “disabled on Linux” status is superseded.
- [ ] Compare the black reference views of
      `level01_07_lead_mine_shafts_A/B/C` with the original. Fog and shadows
      were ruled out in the earlier investigation.
- [ ] Play-test Dog behaviour on each difficulty: hiding round corners, sound
      lures, heavy props, closed doors and attack/get-up transitions. Check
      navigation, ragdolls and animation after the shared engine fixes.
      Automated original-asset tests cover stability, not encounter balance.
      See [Dog foundations](docs/dog-foundations.md).

## Widescreen and usability (Milestone 2)

The wide virtual canvas, full-width overlays, mouse scaling, FOV, gamma,
TrueType fonts and menu display options are implemented. Earlier manual
checks are retained in the archived roadmap; they are not new work here.

- [ ] Decide whether 4:3 menu/logo art should fill the widescreen sides.
- [ ] Blend skeletal animation and particles between fixed logic ticks.
- [ ] Add a frame-rate cap with V-Sync off and avoid spinning while waiting
      for the next simulation step.
- [ ] Apply resolution, fullscreen and display changes without restarting;
      this is also Milestone 3's remaining off-screen-frame convenience item.
- [ ] Add high-DPI/upscaled cursor, inventory, notebook and menu assets.
- [ ] Check the game chooser by hand on Windows and Linux desktops: the
      system folder dialog (XDG portal or zenity on Linux) and its typed-folder
      fallback, high-DPI sizing, GOG detection, and Options → Change Game
      restarting on Windows. Only its offscreen rendering and folder
      detection have been checked so far.
- [ ] Gamepad support (stretch).

## Remaining lighting and material work (Milestones 4–5)

These extend the current improved renderer. Completed implementation details
and original tuning values are in CHANGES and the archived roadmaps.

- [ ] Revisit tonemapping with the current bloom; its shoulder curve was left
      off after the first play-test. Add colour grading if useful.
- [ ] Check the settled bloom values on the outside map, where nearly the
      whole picture was blooming.
- [ ] Evaluate sRGB texture formats for filtering in linear space, only if the
      visible gain warrants it. Preserve colour/data-texture distinctions.
- [ ] Resolve bright MSAA edges being averaged before tonemapping.
- [ ] Add per-material gloss attributes where specular-map inference is wrong.
- [ ] Light transparent materials beyond particles; revisit water and the
      environment-map material (three materials). Dedicated material shaders
      already replaced the generated fixed-function ones in Milestone 6.
- [ ] Check refraction in Overture's acid-water maps and decide its default
      and menu exposure there. Black Plague/Requiem already expose it and
      default it on; do not apply the old “no menu option” claim to them.
- [ ] Consider object velocities for camera motion blur if the absence of
      independently moving-object blur is noticeable.
- [ ] Consider particle shadows, shader detail for small particle images,
      velocity stretching and swirling smoke.
- [ ] Consider a volumetric flashlight cone sampled through its shadow map.

## Further performance work

Profile first. Old GPU timings and reviews in the archive describe earlier
versions and are not current performance baselines.

- [ ] Measure AO after its existing optimization pass. Candidates: fewer
      samples, texture gathers, or reprojection for half-rate rendering
      (the earlier half-rate path ghosted and was disabled by default).
- [ ] Consider compute-based SMAA edge gathering only if its current stencil
      path is a measured bottleneck; retain the supported GL baseline.
- [ ] Profile CPU skinning, pose reuse and dynamic upload ranges before
      proposing GPU skinning. Any GPU path needs shared deformation across
      passes and a CPU bounds/physics/cache policy.
- [ ] Profile live sound-channel lookup and remaining asset-processing costs
      before adding indexes or caches.

Repeated scene-depth resolves, program/material rebinding and unchanged shadow
redraws from the old lists have implementations in steps 11–13; measure those
changes rather than listing them as unimplemented optimizations.

## Black Plague and Requiem

The same executable selects the game through its game chooser, `--game=`,
`OVERTURE_GAME` or the launcher scripts.
Each has separate settings/saves and writable caches. See the
[porting guide](docs/black-plague-port.md) for binary inspection, scripted
checks and bulk map diagnostics.

“Implemented” below means code exists. The old porting roadmap recorded smoke
coverage for 26 Black Plague maps, 12 Requiem maps and 25 Overture maps, plus
specific headless experiments. It did not establish playable end-to-end parity.

### Implemented porting steps 0–4

| Step | Coverage in the earlier porting work | Validation still needed |
| --- | --- | --- |
| 0: startup | Game detection, Requiem resources, script registration, map loading, personal caches | Progression through real entry links, particularly vents |
| 1: traversal | Levers/wheels, locked doors/links, moving and teleporting entities, collisions, stick/vent/damage areas, positional radio, player callbacks | Mechanism feel, transitions and audible behaviour |
| 2: enemies | Infected and its 13 states, weapons/flashlight/grab, enemy-base changes, security cameras, saves | Visible combat/navigation, doors, camera alerts and continued play after load |
| 3: terminals | All 12 Black Plague and 3 Requiem screens opened headlessly; widget input, pop-ups, text, scripts, widescreen, subtitles/radio | Each terminal in context with sound; optional shooter remains a stub |
| 4: remaining systems | Sector EFX, gas mask/collectables, swimming, effects, endings, menus/credits/statistics, object rotation, batteries, autosaves, sector/entity state persistence | Manual play, audio and loaded-game continuity |

### Progression and mechanics checks

- [ ] Play Black Plague from start to end, including its tutorial.
- [ ] Play Requiem from start to end; investigate its binary-only additions
      (14 functions beyond Black Plague were noted, including `StartRecordDump`).
- [ ] Enter `level03_messhall_entrance:link01` through level 2's vent and
      compare with the original: a directly spawned standing player fell
      through the floor. Test vent entry/crouching in levels 2, 3 and 20.
- [ ] Pull/turn/stick levers and wheels, including Requiem level 6's
      wheel-driven joint. Check spring-loaded tower levers and sliding doors
      after the Newton stop fix, including Requiem level 2's security doors
      and their movement sounds.
- [ ] Try locked swing doors and doors held by stick areas.
- [ ] Test positional radio range, callbacks and terminal Escape with sound.
- [ ] Audit missing-file warnings, menu music and names differing from Overture.
- [ ] Check Infected appearance, animations, weapon, flashlight, throat grab,
      flinch/healing and door breaking. Earlier headless hunt frames were too
      dark to establish appearance. Compare level 7's locker ambush walking
      into the wall beside its doorway with the original.
- [ ] Exercise security camera alert behaviour (especially Requiem level 2).
- [ ] Continue from saves containing hunting Infected, alerted cameras,
      stuck levers, end-stopped wheels and moving entities. Headless
      round-trips exist, but resumed play remains unchecked.
- [ ] Verify sector reverb by ear, the gas mask's image/breathing/muffling,
      poison feedback and collectable counts.
- [ ] Swim in Black Plague level 5 and Requiem levels 2, 4 and 9: movement,
      surfacing, oxygen pulse/gasps, drowning and ladder exits.
- [ ] Test object rotation with C, inertia, running while holding and
      holstering a flare; verify pushing/releasing and off/deleted entities.
- [ ] Test flashlight batteries via action, combining and the meter; check
      inventory text, hiding near enemies and spider/worm attack callbacks.
- [ ] Play both endings (Black Plague level 22, Requiem level 0), check credits
      music/text/statistics after actual progression and both secret sequences,
      including Requiem's typed transmission with sound.
- [ ] Check tutorial prompts, Requiem quicksave/quickload and loading screens
      in normal progression, plus Lighting and quality options on both games.

### Remaining script stubs and optional features

`game/core/GameScriptsNotPorted.cpp` currently registers four counting stubs.
Script compilation alone cannot prove these behaviours work.

- [ ] `SetupButton` / `SetButtonOn`: implement `cGameButton` if needed. No
      shipped entity/script use was found in the earlier asset audit.
- [ ] Investigate and implement `StartRecordDump` where required by Requiem.
- [ ] Optional: `cShooter` / `StartSchmup`, the level 18 terminal shoot-'em-up.
      `StartSchmup` currently returns false (“wrong password”).

## Later

- [ ] Reconsider Newton or another physics engine only if parity tuning is
      insufficient; account for all three games' mechanisms and save formats.
- [ ] Packaging (for example Flatpak) and an installer that finds game data.

When updating this plan, distinguish implementation, automated regression
coverage, visual comparison, manual play and measured performance. Preserve
unresolved work when archiving a completed plan; keep historical step numbers
stable for CHANGES and existing validation records.
