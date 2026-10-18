# Code readability audit and cleanup plan

Status: audit started on 2026-10-18. The FadeHandler pilot, RadioHandler
positional-distance batch, systems pass and UI pass are verified by the default
Linux build and both CTest suites. All 20 `game/systems/` and 26 `game/ui/`
headers/sources have been reviewed for naming/comments; A2 is complete.
All 36 `game/entities/`, 26 `game/player/` and 12 `game/enemies/`
headers/sources are reviewed; A3 is verified by the default build and both
CTest suites. A4 core/persistence is now verified; the wider engine/tool inventory and A5–A7 remain open.

Improve readability through subsystem-sized passes, organized into small,
behaviour-preserving batches. A folder is a starting point; declarations,
definitions, callers and tests may span several folders and should change
together.

[ROADMAP.md](ROADMAP.md) remains the project roadmap and priority list. This
document tracks the readability audit and cleanup workflow; it does not replace
the outstanding gameplay and renderer validation work. Follow
[AGENTS.md](AGENTS.md) for repository conventions and detailed checks.

## Scope and conventions

- Preserve Frictional's conventions: `cFoo` classes, `iFoo` interfaces,
  `mpMember` pointers and typed argument prefixes matching surrounding code.
- Prefer names that explain purpose, units and ownership. Distinguish a
  duration from a timestamp, bytes from MiB, and an index from a count or mask.
- Use `.clang-format`: tabs, Allman braces and `if(...)`. Format changed lines;
  any broader formatting should be an explicitly scoped, separate commit.
- Match parameter names between declarations and definitions. Rename files
  when that makes their relationship to the class clearer, updating includes,
  build references, tools and current documentation in the same batch.
- Keep established names when their meaning is already clear. Avoid changing
  every `Handler` suffix, prefix or abbreviation merely for uniformity.
- Preserve script signatures, save field/class identifiers, settings keys,
  asset names, updater registration names, `OVERTURE_*` variables and profile
  paths. Inspect serialization and registration macros before symbol renames;
  a C++ identifier may also become a persisted string.
- Preserve behaviour across Overture, Black Plague and Requiem. Keep existing
  game-selection branches and renderer ownership/state contracts.
- Exclude `vendor/`, generated files and historical documents in `docs/archive/`
  from cleanup sweeps. Historical names can remain in archived reports.

## Readability standard

A reader should be able to find an implementation, understand its purpose and
identify its important contracts without reconstructing the whole subsystem.
Apply the same standard for a human browsing the tree and an agent reading a
header, function or search result with limited surrounding context. Prefer
precise names and nearby contract comments over extra prose or repeated context.
Do not add agent-specific instructions to source comments.

### Functions and methods

- Use a verb that describes the operation and a noun that identifies its target.
  Distinguish queries, mutations, scheduling and immediate execution where the
  distinction matters. Keep established lifecycle and interface method names.
- Make boolean queries readable at their call sites, using existing patterns
  such as `Is`, `Has` or `Can`. Do not rename overrides independently of their
  interface or introduce speculative APIs just to improve a name.
- State non-obvious units, coordinate spaces, ownership transfer, failure
  results and side effects in names or a short declaration comment. Read the
  implementation and callers before claiming any of these contracts.
- Keep declaration and definition argument names consistent. Replace opaque
  arguments such as `abX` when their purpose is known. Leave conventional short
  loop indices and mathematical components alone when their scope is clear.

### Classes, interfaces and files

- Name a class for its actual responsibility and an interface for its contract.
  Investigate vague `Manager`, `Handler`, `Helper` or `Data` names only when they
  obscure that responsibility; suffixes alone are not findings.
- Make the principal class discoverable from its header/source basename using
  the surrounding convention (`cFadeHandler` in `FadeHandler.h` / `.cpp`). A
  file containing related small types can use their shared concept instead.
- Check whether class and file names imply the wrong backend, scope or owner.
  Renames must update include guards, includes, build lists, tests, tools and
  current documentation. Check case-sensitive paths and Windows references.
- Add a concise class or file comment only when the responsibility, boundary
  or lifecycle is not clear from the declaration. Avoid repeating the filename
  or adding a boilerplate header to every file. Preserve copyright/licence text.

### Variables, arguments and constants

- Explain the value's role, not just its type. Distinguish elapsed time,
  duration, rate and timestamp; index and count; capacity and used size; flags
  and masks. Include units when the context could otherwise mislead.
- Make coordinate space and resource role clear where several coexist: world
  and screen positions, scene and output targets, owned and borrowed resources.
  A pointer prefix alone does not establish ownership; verify its lifecycle.
- Use specific boolean names and consistent terms for the same concept across
  declarations and direct callers. Avoid negatives that make conditions hard
  to read, unless they represent an established external setting or contract.
- Keep names proportional to scope. Do not expand familiar `UV`, `GL` or local
  `i` mechanically, invent long sentence-like identifiers, or replace clear
  established names merely to increase consistency.
- Flag unexplained constants for investigation. Naming an existing constant is
  a separate scoped change when justified; changing its value belongs in a
  behaviour change, not this cleanup.

### Concise comments

- Explain intent, constraints or a surprising choice. Remove comments that
  only narrate an obvious assignment, repeat a name or no longer match code.
- Prefer one short sentence. Use a brief paragraph or list when several
  contract details are necessary; concision must not erase essential context.
- Put caller-facing contracts at the declaration and algorithm-specific reasons
  beside the relevant implementation. Avoid copying the same explanation into
  both places; follow existing documentation syntax where useful.
- Preserve explanations of compatibility, ordering, lifetimes, units, error
  handling and renderer state. Verify a historical explanation before rewriting
  it; do not invent a rationale for unusual code.
- Keep TODOs actionable: identify the unresolved issue and relevant condition
  or reference. Remove stale TODOs only after checking that they are resolved.
- Remove empty documentation blocks and redundant section scaffolding in a
  scoped comment/formatting batch. Keep section labels that help navigate a
  large file. Do not replace removed noise with a comment on every symbol.

## Audit method and records

Audit first-party C++ headers/sources, shaders, tests, tools, launchers and
source generators. Apply each language's existing naming conventions; C++
prefixes do not extend to Python or GLSL by default. Exclude vendored and
generated code, historical reports and third-party reference shader code
(including `shaders/SMAA.hlsl`). Preserve authored licences everywhere.

For each bounded area, inspect the header, implementation, representative
callers, registration/serialization sites and relevant tests. Search is a way
to locate candidates, not proof of a naming problem. Confirm each finding in
context. Mark clearly written areas as reviewed without changes.

Record findings here in small tables grouped by area. Each actionable entry
must contain:

| Field | Required content |
| --- | --- |
| ID and status | Stable ID; `open`, `in progress`, `implemented / validation pending`, `verified`, `deferred` or `retained` |
| Location and evidence | Relative file link, symbol and a concrete ambiguity or misleading comment; line numbers are optional and may drift |
| Proposed change | Exact `old -> new` mapping, comment correction or extraction boundary, with the readability benefit |
| Dependencies and risk | Declarations, definitions, callers, strings/macros and build references; low, medium or high risk with a reason |
| Validation | Required checks, actual results and any unavailable checks |
| Resolution | Implementation reference when available; reasons for retaining or deferring an item, and any follow-up condition |

Use low risk for local names/comments without external coupling, medium for
cross-file names or file/build changes, and high for exposed serialization,
callbacks, shared ownership or renderer state. A rename in a small file can
still be high risk. Prioritize misleading contracts, units and ownership over
cosmetic consistency. Defer changes whose compatibility cannot yet be proved.

Do not mark an entire directory audited after inspecting a sample. Track the
reviewed files/concepts and remaining scope for every phase. A reviewed area
may legitimately produce no findings. Findings are not a target rename count.

### Seed findings: FadeHandler

Inspected and renamed on 2026-10-18, including direct callers and
registration/serialization searches. Retained the class and file names: they
already identify the responsibility and match each other.

| ID / status | Evidence | Proposal and reason | Risk / remaining work |
| --- | --- | --- | --- |
| R01 / verified | [FadeHandler.h](game/systems/FadeHandler.h), `SetWideScreenActive(bool abX)`; definition assigns `abX` to `mbWideScreenActive` | `abX -> abActive` in declaration and definition; state the boolean's meaning | Low; references checked; default build and both CTest suites passed |
| R02 / verified | [FadeHandler.cpp](game/systems/FadeHandler.cpp), `FadeIn` / `FadeOut` compute `1.0f / afTime` | `afTime -> afDurationSeconds`; make duration and units explicit | Low; seconds confirmed via logic timer/update path and callers; nonpositive-duration handling and immediate update preserved |
| R03 / verified | [FadeHandler.cpp](game/systems/FadeHandler.cpp), `Update` multiplies `mfAlphaAdd` by `afTimeStep` | `mfAlphaAdd -> mfAlphaRatePerSecond`; distinguish a signed rate from an increment | Low; all member uses checked; timestep seconds confirmed; arithmetic and thresholds preserved |

For these entries, `"FadeHandler"` is an updater registration string and must
remain unchanged. The fixed initial update and differing nonpositive-duration
rates are existing behaviour; investigate separately if needed. No rationale
comment should be added without evidence. Validation results for this batch are recorded in the A1 phase record below.

## Work scope and batch workflow

Default to a subsystem-sized scope per turn. Inspect the selected subsystem,
apply justified naming and comment cleanups across its related files, validate
the resulting changes and update the findings before handing back. Small
batches organize reviewable changes within that work; several batches can be
completed in one turn. Completing a few renames is not by itself a handoff
point, and continuing within the agreed scope needs no fresh confirmation.

Retain names that are already clear. Resolve routine choices from the code and
callers. Isolate and record findings with unresolved compatibility risks so
independent work can continue; ask only when a material ambiguity or risk
requires user input. Report any remaining scope explicitly.

The systems and UI passes under A2 and the completed A3 mechanism/state
reviews in entities, player and enemies are recorded below. Next is A4: core
and persistence, retaining compatibility-sensitive identifiers.
Keep findings grouped by responsibility and risk without turning each group
into a separate user turn.

1. Inspect status and relevant diffs; preserve unrelated work. Read the header,
   implementation, callers, current guide and relevant tests before choosing
   a change. Search uses with `rg`, including strings and macros.
2. Write a concrete objective and list affected files. For renames, record
   `old name -> new name` and the reason. For refactors, identify the current
   responsibility, intended boundary and contracts that must survive.
3. Make one type of change per commit: comments/formatting, naming, or structural
   refactoring. Small adjacent comment corrections are fine. Keep bug fixes,
   algorithm changes and tuning separate from cleanup.
4. Review the diff for unintended changes to conditions, constants, update
   order, lifetimes, serialization and public strings. Check old-name matches
   individually; archive references and compatibility identifiers may remain.
5. Run the applicable validation below. Compatible naming/comment batches can
   share a build and regression run after the subsystem pass; validate riskier
   changes separately when needed. Add focused regression coverage for
   an uncovered behaviour when a structural refactor puts it at risk; avoid
   tests that merely mirror a rename or formatting edit.
6. Update current documentation when names or architecture change. Record
   implemented changes and actual validation in CHANGES. Keep coherent batches
   reviewable as work continues; create commits only when requested.

If the diff becomes difficult to explain or review, split the batch. Do not
combine a subsystem cleanup with a repository-wide naming migration.

## Milestones and phases

These milestones belong to this audit; they do not renumber the engine/game
milestones in ROADMAP. Each implementation phase consists of small batches,
grouped into subsystem-sized passes that can complete several batches per
turn. Commit boundaries do not define turn boundaries. The starting areas
below are inspection candidates, not claims that those files are faulty.

| Phase | Scope and deliverable | Exit criteria | Status |
| --- | --- | --- | --- |
| A0: inventory and criteria | Apply the standards above; list reviewed and remaining areas; expand the seed findings with evidence, dependencies and risk | Initial work queue has concrete bounded batches and required checks; uninspected areas are explicit | Started: systems/UI/entities/player/enemies inventoried; wider scope open |
| A1: pilot | Audit `game/systems/FadeHandler.h` / `.cpp`, direct callers and compatibility sites; resolve R01–R03 or record reasons to retain them; inspect comments separately | First naming batch validated; comment decisions recorded; actual results establish the pattern for later batches | Verified |
| A2: systems and UI | Review remaining `game/systems/`, then `game/ui/`; inspect a contained system such as RadioHandler and one screen interaction at a time | Reviewed scope recorded; actionable findings resolved or deferred with reasons; timing, coordinates, callbacks and resource roles remain clear | Verified: all 20 systems and 26 UI files reviewed |
| A3: entities, player and enemies | Review `game/entities/`, `game/player/` and `game/enemies/` by mechanism/state; audit names and transition comments | State/callback/physics findings dispositioned; save/script strings checked; applicable smoke, Dog, sanitizer and manual results recorded | Verified: 36 entity, 26 player and 12 enemy files reviewed |
| A4: core and persistence | Review `game/core/` map/save/script helpers, then bounded sections of `cInit`; document registration and lifetime contracts where needed | Compatibility-sensitive identifiers mapped and retained where required; game-selection, save and initialization contracts checked | Verified; see A4 record |
| A5: shared engine and shaders | Review focused APIs in `engine/include/` and `engine/sources/`, then their first-party shader counterparts and callers | API/class/file/argument names agree; ownership and state comments checked; applicable engine, EGL, supported GL paths and visual results recorded | Open |
| A6: tests, tools and build surfaces | Review `tests/`, `tools/`, `testmap/`, root build/launch files and `platform/windows/`; keep fixtures and diagnostics discoverable | Test/helper/file names explain coverage; commands, imports, includes and current documentation agree; affected platform checks recorded | Open |
| A7: closure | Reconcile the inventory, findings, current guides and CHANGES; review any justified structural extractions separately | All in-scope areas reviewed; every finding has a disposition; validation gaps and deferred work remain visible | Open |

A0 inventories the whole scope but does not require a full audit before A1.
Expand the inventory as each phase starts. A2–A6 can be reordered when a
concrete dependency justifies it; update the record rather than leaving an
implied dependency. Engine work need not wait for every game folder. Review
supporting tools/tests in the same batch when a rename reaches them, then use
A6 to inspect their remaining scope.

For renderer work, distinguish high-level `graphics/` policy, `scene/` objects
and concrete `impl/` resource owners. Preserve those boundaries. Recently
renamed texture, lamp, renderer-pass and shadow-cache code already has cleanup
recorded in CHANGES; inspect the current tree before proposing more renames.

### Phase tracking

For every phase, maintain a short record with these entries:

- Reviewed scope: exact files and concepts, including those retained unchanged.
- Remaining scope: unreviewed files/subsystems; link a larger inventory if needed.
- Findings: IDs, next bounded batch and reasons for any changed priority.
- Evidence: completed checks, limitations and outstanding validation.
- Exit decision: open, implementation complete with validation pending, or
  verified; identify any explicitly deferred work.

Coverage includes all 20 headers/sources in `game/systems/` and all 26 in
`game/ui/`, with reviewed concepts, callers and retained names recorded below.
A3 covers all 36 entity, 26 player and 12 enemy files through the records
below, including the ordinary and HaptX player state paths. A4–A6 remain
unreviewed except for supporting callers inspected in these passes.

### A1: FadeHandler pilot record (2026-10-18)

- Reviewed scope: `FadeHandler.h` / `.cpp`; direct uses in `GameScripts.cpp`,
  `MapHandler.cpp`, `RefViewEffects.cpp` and `Ending.cpp`; construction,
  update registration and destruction in `Init.cpp`; repository references
  and existing regression coverage. `RefView.cpp` only includes the header.
- Implemented R01–R03 in the header and implementation. Call expressions and
  script wrappers require no edits. `cLogicTimer::GetStepSize()` converts
  milliseconds to seconds and `cGame` passes that step to the updater;
  fades use reciprocal duration and integrate the signed rate with that step.
- Compatibility: no FadeHandler serialization macros or save fields were
  found. Class, method, updater and script registration names stay unchanged,
  including `"FadeHandler"`. Existing nonpositive-duration constants, initial
  1/60-second update, comparisons and drawing operations are preserved.
- Retained names: class/file names, lifecycle methods, public method names and
  scoped locals already describe their roles. Retained comments and section
  labels for this naming-only batch; no unsupported rationale was added.
- Evidence: exact comparison against the starting source confirms only the
  three identifier substitutions. `cmake --build --preset default` passed;
  `ctest --test-dir build --output-on-failure` passed both `engine_regressions`
  and `renderer_regressions` (software EGL, no skips). Sanitizer builds,
  game smoke/manual play, visual captures and Windows checks were not run
  for this identifier-only change.
- Remaining scope: all later phases; no directory-wide audit is claimed.
  Existing tests have no direct FadeHandler coverage; this batch changes no
  behaviour and adds no rename-only tests.
- Exit decision: verified for this naming-only batch.

### A2: RadioHandler positional-distance naming (2026-10-18)

Scope: [RadioHandler.h](game/systems/RadioHandler.h) and
[RadioHandler.cpp](game/systems/RadioHandler.cpp), including message creation,
positional volume calculation, playback/subtitle updates and world exit.

| ID / status | Evidence | Implemented mapping and benefit | Dependencies / risk |
| --- | --- | --- | --- |
| R04 / verified | Message distance fields and positional `Add` arguments describe bounds without their volume roles; `GetVolumeAtPlayer` returns full volume at the inner bound and fades to zero at the outer bound for ordered ranges | `mfMinDist -> mfFullVolumeDistance`, `mfMaxDist -> mfSilentDistance`; corresponding `afMinDist -> afFullVolumeDistance`, `afMaxDist -> afSilentDistance` | Low; field uses are confined to RadioHandler, arguments match declaration/definition, script wrapper call order unchanged |
| R05 / verified | `GetVolumeAtPlayer` uses `vPos` and `fDist` without naming the endpoints | `vPos -> vSourcePosition`, `fDist -> fDistanceToPlayer`; identify source position and distance from the player camera | Low; local identifiers only; coordinate accessors and arithmetic unchanged |

- Caller/compatibility review: `GameScripts.cpp` queues ordinary/positional
  messages, stops playback and sets map/terminal callbacks; `SaveHandler.cpp`
  saves/restores `msOnRadioEndCallback`; `Init.cpp` owns/registers the handler;
  `MapHandler.cpp` calls `OnWorldExit`; `ButtonHandler.cpp` advances terminal
  messages; `EffectHandler.cpp` and `Inventory.cpp` query subtitle visibility.
  Repository searches found no external uses or serialization of the renamed
  message fields. Script signatures, wrapper parameter names, persisted callback
  identifiers and the `"RadioHandler"` updater string remain unchanged.
- Retained: class/file names and public methods already describe their roles
  in context. Existing comments explain positional playback, callback clearing
  and game-specific visibility. No comment or structural changes in this batch.
- Evidence: exact comparison with starting source confirms only the six
  identifier substitutions. `cmake --build --preset default` passed, with
  missing-initializer warnings in unchanged `ButtonHandler.cpp`.
  `ctest --test-dir build --output-on-failure` passed both suites, including
  software EGL renderer regressions with no skips. Documentation links and
  `git diff --check` passed. Sanitizer builds, game smoke/manual play, audio
  checks, visual captures and Windows checks were not run for these renames.
- Remaining scope: RadioHandler callback, queue and subtitle naming beyond
  this positional-distance batch; the remaining systems and UI files.
  Existing regressions have no direct radio coverage; no rename-only tests
  were added. This does not establish audible playback or gameplay parity.
- Exit decision: this naming batch is verified; A2 remains open.

### A2: Systems naming and comments pass (2026-10-18)

Reviewed all headers and implementations for `AttackHandler`, `EffectHandler`,
`FadeHandler`, `GameMessageHandler`, `GameMusicHandler`, `GameSoundEnvHandler`,
`PhysicsLog`, `RadioHandler`, `TriggerHandler` and `Triggers` (20 files).
Eight pairs changed in this pass, plus the message-focus caller in
[GameEntity.cpp](game/entities/GameEntity.cpp). FadeHandler and Triggers were
retained as inspected. This completes the systems naming/comment pass, with
no claim of gameplay, audio or ownership correctness from this review.

| ID / status | Location and finding | Implemented mapping / resolution | Risk |
| --- | --- | --- | --- |
| R06 / verified | [RadioHandler](game/systems/RadioHandler.h): waiting messages, text alpha and callback destination need distinct roles | `mlstMessages -> mlstQueuedMessages`, `mfAlpha -> mfTextCrossfadeAlpha`, `mbGuiCallback -> mbOnEndCallbackInGui`, `abGui -> abCallbackInGui`, `asFunc -> asFunction`, `pMess -> pMessage`; removed two comments that merely narrated dequeuing/starting playback | Low; header/source only, public strings and callback execution order unchanged |
| R07 / verified | [GameMessageHandler](game/systems/GameMessageHandler.h): misspelled focus flag hides the depth-of-field cleanup contract; fade increment is a signed rate | `SetFocusIsedUsed -> SetDepthOfFieldUsed`, `mbFocusIsedUsed -> mbDepthOfFieldUsed`, `abX -> abUsed` in that setter; `mfFade -> mfTextAlpha`, `mfFadeAdd -> mfTextAlphaRatePerSecond`; `mpMessHandler/apMessHandler -> mpMessageHandler/apMessageHandler`; `pMess -> pMessage`, `pPrevMess -> pPreviousMessage`, `bFoundMess -> bFoundNextMessage`; `abX -> abBlackText` in `SetBlackText` | Medium; updated the sole focus setter caller in GameEntity; no script or save registration uses the renamed symbols |
| R08 / verified | [GameMusicHandler](game/systems/GameMusicHandler.h): four `Count` members accumulate timestep seconds, rather than events | `mfAttackPlayCount -> mfAttackStartElapsedSeconds`, `mfAttackStopCount -> mfAttackStopElapsedSeconds`, `mfEnemyCloseCount -> mfEnemyCloseElapsedSeconds`, `mfEnemyGoneCount -> mfEnemyGoneElapsedSeconds` | Low; private timers only; save DTO fields and music priorities retained |
| R09 / verified | [GameSoundEnvHandler](game/systems/GameSoundEnvHandler.h): elapsed fade time is obscured by `Count` | `mfFadeCount -> mfFadeElapsedSeconds`; retained `mfFadeTime` on sector records and save DTOs, including explicit save/load assignments | Low; only private elapsed timer renamed, serialized fields unchanged |
| R10 / verified | [TriggerHandler](game/systems/TriggerHandler.h): countdown and input lifetime use vague time/count names; radius argument is `afX` | `mfTimeCount -> mfRemainingSeconds`, `GetTimeCount -> GetRemainingSeconds`, `afTime -> afLifetimeSeconds`, `afX -> afRadius` | Medium; API rename checked across first-party callers/subclasses; no getter callers or serialization found; dispatch and expiry order retained |
| R11 / verified | [AttackHandler](game/systems/AttackHandler.cpp): `CalcSize` computes distance falloff for damage/force; maximum damage is misspelled | `CalcSize -> CalculateRadialFalloff`, `afMaxDamge -> afMaxDamage`, `a_mtxOffset -> a_mtxShapeOffset`; corrected bounding-box and body-snapshot comments | Low; static helper and argument names only; physics/damage operations unchanged |
| R12 / verified | [PhysicsLog](game/systems/PhysicsLog.h): generic time and abbreviated formatting helpers obscure diagnostic roles | `mfTime -> mfMapTimeSeconds`, `VecStr -> FormatVector`, `JointInfo -> GetJointDescription` | Low; private diagnostic names only; log text, cadence and body tracking retained |
| R13 / verified | [EffectHandler](game/systems/EffectHandler.h): cooldowns, elapsed time, target alpha and duration inputs need distinct meanings | `mfSoundCount -> mfDiveSoundCooldownSeconds`, `mfBreathCount -> mfBreathElapsedSeconds`, `mfMaxAlpha -> mfTargetAlpha`, `mfAlphaAdd -> mfAlphaRatePerSecond`; flash/image-flash arguments `afFadeIn/afWhite/afStay/afFadeOut -> afFadeInSeconds/afWhiteSeconds/afStaySeconds/afFadeOutSeconds`; setter `abX -> abActive`, `abDisabled` or `abBlackText` according to its target | Low; declarations/definitions agree, callers and script signatures retained; gravity comment now correctly describes waking dynamic bodies |
| R14 / verified | [GameMessageHandler.cpp](game/systems/GameMessageHandler.cpp), `ShowNext`: comment said fully visible but the guard tests 0.2 opacity | Comment now states that advancing is allowed at 20% opacity | Low; threshold and condition unchanged |

Compatibility and caller evidence:

- Read `GameEntity::OnPlayerExamine` and updated its depth-of-field flag setter;
  reviewed message cleanup in `ShowNext`. Read radio/subtitle callers and
  persistence mappings in the preceding RadioHandler batch; this pass keeps
  those public methods and stored callback strings.
- Reviewed script forwarding for flash/image flash, underwater-related effects,
  dream filter, gravity, depth of field, messages and splash damage in
  `GameScripts.cpp`, plus effect controls in `RefViewEffects.cpp`. Wrapper names,
  parameter order, argument values and registration macros are unchanged.
  The wrapper's local `afMaxDamge` remains outside this handler batch.
- Read sound-trigger construction in `MapHandler.cpp` and `Player.cpp`, and
  dispatch/consumption in `GameEnemy.cpp`. Trigger ownership, priority ordering,
  body/entity access and update timing are unchanged.
- Checked music and sound-environment save DTOs and serialization macros in
  `SaveHandler.h` / `.cpp`. Music timers are not stored there; sector fade
  durations are stored under their existing names. No save schema edits.
- Confirmed seconds through `cLogicTimer::GetStepSize` and timestep arithmetic.
  Normalized flash hold counters, gravity phase, short local math names,
  established class/file names and lifecycle methods are retained where their
  surrounding implementation explains them. `Triggers.cpp` has no substantive
  implementation to rename; `cGameTrigger_Sound` and `mpSound` identify their
  roles. Historical disabled code and ownership/state policy were not rewritten.

Validation and exit:

- Default Linux build passed, with existing nested-comment warnings in
  unchanged `GameEnemy_Worm.cpp`. Both CTest suites passed (2/2), including
  software EGL renderer regressions with no skips.
- Token comparison against a saved starting snapshot of all 20 systems files
  and GameEntity confirmed only the recorded identifier substitutions, comments
  and whitespace; string/character literals, operations and constants match.
  Searches checked removed names and scope-specific uses, including unrelated
  `GetTimeCount` APIs that must remain unchanged.
- Local documentation links and `git diff --check` passed. No new tests mirror
  the renames. Existing CTest suites do not directly exercise these game
  systems; Dog fixtures call the unchanged attack APIs but were not run.
  Sanitizer builds, isolated game smoke, Dog checks, audio/manual play, visual
  captures and Windows checks were not run for this naming/comment pass.
- Systems naming/comment review was complete at this point; the following UI
  pass closes A2. Later phases and gameplay/renderer validation in ROADMAP
  remain open.

### A2: UI naming and comments pass (2026-10-18)

Reviewed all 26 headers/sources: `Credits.h` / `.cpp`, `DeathMenu.h` / `.cpp`,
`DemoEndText.h` / `.cpp`, `Ending.h` / `.cpp`, `GameGui.h` / `.cpp`,
`GraphicsPresets.h` / `.cpp`, `IntroStory.h` / `.cpp`, `Inventory.h` / `.cpp`,
`MainMenu.h` / `.cpp`, `MapLoadText.h` / `.cpp`, `Notebook.h` / `.cpp`,
`NumericalPanel.h` / `.cpp` and `PreMenu.h` / `.cpp`. Reviewed activation/input,
menu states/options/save lists, intro cues/image fades, typed endings/secrets,
loading prompts, terminal widgets/pop-ups/callbacks/deferred closure, inventory
slots/items/combining/persistence and notebook pagination/handoff/persistence.
`GraphicsPresets.h` is retained unchanged; 25 files have naming/comment edits.
All findings below use the shared validation evidence following the table.

| ID / status | Location and evidence | Implemented mapping / resolution | Risk |
| --- | --- | --- | --- |
| R15 / verified | UI activation declarations and definitions use opaque `abX`; [Inventory.h](game/ui/Inventory.h) also hides slot/count setter roles | `abX -> abActive` for activation; inventory setters use `abEquip`, `abNotebookActive`, `abDroppedInSlot`; `alX -> alIndex`, `alEquipIndex`, `alAmount`, `alCount` in their respective setters | Low; argument names only; method names and callers retained |
| R16 / verified | [DeathMenu.h](game/ui/DeathMenu.h), [NumericalPanel.h](game/ui/NumericalPanel.h) and [MainMenu.h](game/ui/MainMenu.h) contain `mvPositon`; new-game difficulty and key-widget arguments are misspelled | `mvPositon -> mvPosition` in death/numerical buttons, `mvPositon -> mvWidgetPos` in main-menu widgets; `mDiffuculty/aDiffuculty -> mDifficulty/aDifficulty`, `apKeyWiget -> apKeyWidget` | Medium; inherited member uses checked; main-menu list's distinct `mvPosition` retained to avoid changing member binding/depth |
| R17 / verified | [IntroStory.h](game/ui/IntroStory.h) calls both brightness and alpha `afDarkness`; steps are multiplied by timestep and cue count is decremented | Header `afDarkness -> afBrightness/afAlpha` matches definitions; movement/fade `afTime -> afDurationSeconds`; `mvPosStep -> mvVelocity`, `mfBrightnessStep -> mfBrightnessRatePerSecond`, `mfAlphaStep -> mfAlphaRatePerSecond`, `mfTimerCount -> mfCueRemainingSeconds` | Low; image-local names and private cue timer; zero-duration handling, clamping, smoothing and cue order retained |
| R18 / verified | [Credits.h](game/ui/Credits.h): page elapsed time, typing delays and secret text index have generic time/number names | `mfPageTime -> mfPageElapsedSeconds`, `mfCharTime -> mfCharacterDelaySeconds`, `mfTypeSoundTime -> mfTypeSoundDelaySeconds`, `mlSecretText -> mlSecretTextIndex`, `alNum -> alTextIndex`; removed orphan config-loading comment | Low; private state only; statistics, collectable gates, translations and secret codes retained |
| R19 / verified | [Ending.h](game/ui/Ending.h): three `Count` members measure waits/delays; typing comment implies a pause after every row | `mfCount -> mfWaitElapsedSeconds`, `mfCharCount -> mfCharacterDelaySeconds`, `mfTypeSoundCount -> mfTypeSoundDelaySeconds`; comment now states the pause is before an empty row | Low; private timers; ending selectors, click skips, script return/credits transitions and sound cadence retained |
| R20 / verified | [DemoEndText](game/ui/DemoEndText.h) and [PreMenu](game/ui/PreMenu.h) pass `mfAlpha` as grayscale brightness with opacity fixed at 1 | Demo `mfAlpha -> mfImageBrightness`, `mfAlphaAdd -> mfBrightnessRatePerSecond`; premenu `mfAlpha -> mfStartupLogoBrightness`, `mfAlphaAdd -> mfLogoBrightnessRatePerSecond`, `mfClickCount -> mfTypeSoundDelaySeconds`, `mfStateTimer -> mfStateElapsedSeconds`, `mfAdd -> mfLogoScaleStep` | Low; private visual/timing names; logo scale step already includes timestep and remains a step, not a rate |
| R21 / verified | [MapLoadText.h](game/ui/MapLoadText.h): `mfAlpha` controls colour and sinusoidal offsets, not opacity; `mpBack` is a texture | `mfAlpha -> mfPulsePhase`, `mfAlphaAdd -> mfPulseRatePerSecond`, `mpBack -> mpBackgroundTexture` | Low; private names; oscillation bounds, background selection and immediate loading draw retained |
| R22 / verified | [GameGui.h](game/ui/GameGui.h): `mbDestroyMe` and `abLater` obscure a deferred screen-close request | `mbDestroyMe -> mbCloseRequested`, `abLater -> abDeferClose`; declaration explains safe closure from the screen script; text-view comment states that the close button queues deletion through the GUI set | Low; header/source uses only; script wrapper still requests deferred closure; focus, callbacks and destruction order retained |
| R23 / verified | [Notebook.h](game/ui/Notebook.h): note page `mlStart/mlEnd` do not identify rows or their boundary convention | `mlStart -> mlFirstRow`, `mlEnd -> mlEndRow` in `cNotePage`; documented exclusive end and excluded page marker; updated disabled diagnostic comments | Low; transient page records only; task-page bounds, note/task DTOs and pagination arithmetic retained |
| R24 / verified | [MainMenu.cpp](game/ui/MainMenu.cpp): `cTempFileAndData` actually holds a save filename/date; state lists, rain timer and haptic/post-effect locals lack roles; [GraphicsPresets.cpp](game/ui/GraphicsPresets.cpp) clamp uses `alX` | `cTempFileAndData -> cSaveFileEntry`, `tTempFileAndDataSet/It -> tSaveFileEntrySet/It`, `setTempFiles -> setSaveFiles`, `mvState -> mvStateWidgets`, `mfRainDropCount -> mfRainRefreshRemainingSeconds`, haptic `afX -> fForceScale/fCameraSpeed`, `bX -> bPostEffectsActive`; clamp `alX -> alAnisotropy` | Low; implementation-local helper and private names; date ordering, widget membership, settings values and limits retained |
| R25 / verified | [Inventory.h](game/ui/Inventory.h): `Check*Callback` methods execute scripts; death/numerical draw/input loops are labelled updates | Added callback dispatch contract; clarified disabled right-click context menus; corrected button loop labels to drawing/press/release dispatch | Low; comments only; no callback API or execution changes |
| R26 / retained | Inventory callback `bKillMe`, inventory/notebook DTOs, menu settings, updater names and established UI classes/files | Retained serialized names, including `bKillMe` and `mbNoteBookActive`; retained class/file and lifecycle names, state/asset/translation strings and public script signatures | Compatibility-sensitive names intentionally retained; no pending rename |

Compatibility and caller evidence:

- Read UI construction, updater containers and destruction in `Init.cpp`, input
  routing in `ButtonHandler.cpp`, script forwarding/registration in
  `GameScripts.cpp`, loading-screen selection in `MapHandler.cpp`, item actions
  in `GameItemType.cpp` and inventory entry/shortcut calls in `Player.cpp`.
  No caller expressions or script wrappers require edits.
- Read inventory/notebook global save mappings and callback serialization in
  UI sources and `SaveHandler.h` / `.cpp`, plus callback records in
  `GameTypes.h`. All serialization invocations and arguments are unchanged.
  Main-menu save file listing still sorts by date with the same comparator;
  no save paths, file operations, format strings or settings identifiers change.
- Confirmed brightness through `cColor(float afVal, float afA)` and draw uses;
  timing units through `cLogicTimer::GetStepSize`, the updater and timestep
  arithmetic. Read `iGuiPopUp::SelfDestruct`, `cGuiSet::DestroyPopUp` and queued
  pop-up deletion before correcting the terminal text-view comment.
- Retained short scoped math/loop names, documented preset quality encodings,
  callback macros, notebook task-page inclusive bounds and the terminal's
  fixed 800 x 600 coordinates. Legacy unused declarations/write-only fields
  and disabled code are not removed in this naming pass. This review does not
  establish their correctness or change ownership policy.

Validation and exit:

- Default Linux build passed. Existing missing-initializer warnings in unchanged
  `ButtonHandler.cpp` and a variable-tracking compiler note in unchanged
  `GameScripts.cpp` were observed. Both CTest suites passed (2/2), including
  software EGL `renderer_regressions` with no skips.
- Token comparison of all 26 UI files against `981e71e` confirms 432 identifier
  substitutions across 52 old/new mappings, plus comments/whitespace only.
  String/character literals, operators, constants, serialization invocations
  and updater registration strings match. Reviewed old-name searches and
  inherited-member bindings; local documentation links and `git diff --check`
  passed. No tests were added to mirror renames.
- Existing regressions do not directly exercise these game UI flows. Manual
  menu/inventory/notebook/terminal/ending interaction, audio checks and visual
  captures were not run. Sanitizer and Windows checks were not run for this
  identifier/comment-only pass; no behaviour or renderer path change is claimed.
- Exit decision: A2 systems/UI naming and comment review is verified and
  complete, with no deferred naming findings from this UI pass. A0's wider
  inventory and A3–A7 remain open; ROADMAP's gameplay/renderer validation work
  remains unchanged.

### A3: environmental-area naming and comments pass (2026-10-18)

Objective: clarify environmental-volume configuration, damage scheduling and
force/wave calculations without changing physics, game selection or external
identifiers. Reviewed all eight headers/sources for `GameArea`,
`GameDamageArea`, `GameForceArea` and `GameLiquidArea`. Seven files changed;
`GameArea.h` is retained unchanged. Reviewed loaders, area-body creation,
picking, damage cadence/target selection, force calculation/contact averaging,
liquid camera/body checks, swimming, buoyancy, waves, splash effects and save
mapping. This is a bounded start to A3, not an entities-wide audit.

| ID / status | Location and evidence | Implemented mapping / resolution | Risk |
| --- | --- | --- | --- |
| R27 / verified | [GameDamageArea.h](game/entities/GameDamageArea.h), [GameForceArea.h](game/entities/GameForceArea.h), [GameLiquidArea.h](game/entities/GameLiquidArea.h): setter arguments use `afX`, `alX`, `abX`, `asX` | Damage: `afX -> afDamage/afUpdatesPerSec`, `alX -> alStrength`, `abX -> abDisableObjects/abDisableEnemies`, `asX -> asSound`; force: `afX -> afMaxForce/afSpeedErrorGain/afTargetSpeed/afMaxMass`, `abX -> abMultiplyByMass/abAffectCharacters/abAffectBodies/abForceAtContact`; liquid: `afX -> afDensity/afLinearViscosity/afAngularViscosity`, `abX -> abHasWaves`, each in its respective setter | Low; inline argument names only; public methods and script wrappers unchanged |
| R28 / verified | [GameDamageArea.cpp](game/entities/GameDamageArea.cpp): `mfUpdateCount` is assigned reciprocal update frequency and decremented by timestep | `mfUpdateCount -> mfDamageDelaySeconds` in header/source; distinguish a countdown from an update count | Low; private, nonserialized timer; initial immediate damage, reset/decrement branches, early return and frequency edge cases preserved |
| R29 / verified | [GameForceArea.cpp](game/entities/GameForceArea.cpp): `mvUp` drives force, `fRelSpeed/fDiff` project velocity and subtract it from target speed; `vPos` averages contacts | `mvUp -> mvForceDirection`, `fRelSpeed -> fSpeedAlongForce`, `fDiff -> fSpeedError`, `vPos -> vContactWorldPos`, `vVel -> vBodyVelocity`; declaration comment explains `SetConstant` as proportional speed-error gain | Low; private/local names; inverse-matrix direction calculation, contact limit, gain/clamping/mass order, character multiplier and force application preserved |
| R30 / verified | [GameLiquidArea.cpp](game/entities/GameLiquidArea.cpp): elapsed timestep multiplies sine frequency; offsets modify a temporary buoyancy plane | `mfTimeCount -> mfWaveElapsedSeconds`, `mfWaveAmp -> mfWaveAmplitude`, `mfWaveFreq -> mfWaveAngularFrequency` (radians per second); wave-block `vPos -> vMassCentreWorldPos`, `fAddX/fAddZ -> fWaveOffsetX/fWaveOffsetZ`, `tempPlane -> waveSurfacePlane` | Low; private/local names only; persisted surface plane, wave flag, constants, splash positions and swim branches unchanged |
| R31 / verified | Area loaders contain speculative return comments; candidate loops claim to scan all world bodies; force/liquid closing-guard comments name DamageArea | Explain map-handler ownership and null scene-entity return; retain candidate-snapshot rationale, state bounded candidate iteration, correct force multiplier/guard comments and remove orphan wave heading | Low; comments only; ownership verified in map registration/destruction; no lifecycle change |
| R32 / retained | Area DTOs, serialization/copy macros and script/loader registrations expose configuration identifiers | Retain all DTO/class/member identifiers, including `mfConstant`, `mfDestSpeed`, `mbMulWithMass`, `mbForceAtPoint`, `mfUpdatesPerSec`, material names and `mSurfacePlane`; retain established class/file/method names | Compatibility-sensitive names intentionally retained; no pending rename |

Compatibility and caller evidence:

- Read `GameScripts.cpp` area icon/splash/teleport uses and all three setup
  wrappers, including Black Plague's damage overload and registration alias.
  Their declarations, call expressions, registration signatures and literals
  require no changes. Read loader registration in `Init.cpp` and saved-area
  recreation in `MapHandler.cpp`; `script`, `damage`, `force` and `liquid`
  loader names, entity types, saved sizes/transforms and setup/load order stay
  unchanged. No script, save, settings or asset identifiers change.
- Read all four serialization blocks, DTO declarations and save/load/setup
  methods. The renamed private timers, direction and wave parameters do not
  appear in serialization/copy macros. Compared those macro invocations exactly.
- Confirmed seconds in `cLogicTimer::GetStepSize`; checked Newton collision
  contact output, `iPhysicsBody::GetVelocityAtPosition` and
  `cPhysicsBodyNewton::AddForceAtPosition` before naming contact coordinates.
  Read `AddGameEntity` and entity destruction in `MapHandler.cpp` before
  documenting ownership, and damage/death forwarding in `GameEntity.cpp` /
  `GameObject.cpp` for the candidate-snapshot comment.
- Retained short scoped math/loop names, disabled diagnostics, unused legacy
  declarations and existing error paths. This pass does not fix or validate
  their behaviour. No renderer or physics algorithm changes were made.

Validation and exit:

- Default Linux build and both CTest suites passed (2/2), including software
  EGL `renderer_regressions` with no skips. The existing `GameScripts.cpp`
  variable-tracking compiler note remains. Existing tests do not directly
  exercise these game-area flows; no tests were added to mirror renames.
- Token comparison against this pass's starting sources confirms 90 permitted
  identifier substitutions across the eight reviewed files, plus comments and
  whitespace only. All other tokens, literals and serialization/copy macro
  invocations are unchanged. Old-name searches, local documentation links and
  `git diff --check` passed. All 25 previously modified UI files are byte-for-byte
  preserved; the existing UI documentation record remains in place.
- Sanitizer builds, isolated game smoke/save/reload, manual area/swimming/audio
  checks, visual captures and Windows checks were not run for this
  identifier/comment-only batch. No gameplay, save-compatibility or visual
  validation is claimed beyond source equivalence and the existing suites.
- Exit decision: this bounded environmental-area review is verified; A3 stays
  open. Remaining entity pairs: `GameCamera`, `GameEntity`, `GameItem`,
  `GameItemType`, `GameLadder`, `GameLamp`, `GameLever`, `GameLink`, `GameObject`,
  `GameSaveArea`, `GameStickArea`, `GameSwingDoor`, `GameVentArea` and `GameWheel`
  (28 headers/sources). All `game/player/` and `game/enemies/` remain unaudited.
  The next interaction/traversal pass is recorded below. A0's wider
  inventory, A4–A7 and ROADMAP's gameplay/renderer validation remain open.

### A3: traversal and save-area naming/comments pass (2026-10-18)

Objective: clarify ladder reach/entry queries, vent entry motion and save-area
roles without changing state transitions, collision checks or persistence.
Reviewed all eight headers/sources for `GameLink`, `GameLadder`, `GameVentArea`
and `GameSaveArea`: loaders, picking/interactions, locked feedback/map changes,
ladder entry search and climb handoff, vent entry/latch/crouch/clearance/motion,
save-effect handoff and all serialization mappings. Seven files changed;
`GameLink.cpp` is retained unchanged. This brings A3 to 16/36 entity files.

| ID / status | Location and evidence | Implemented mapping / resolution | Risk |
| --- | --- | --- | --- |
| R33 / verified | [GameLadder.h](game/entities/GameLadder.h), [GameLadder.cpp](game/entities/GameLadder.cpp): `GetHeight` returns picked Y minus player Y, not ladder height; `GetDist2D` measures player distance in XZ | `GetHeight -> GetPickedHeightOffset`, `GetDist2D -> GetPlayerDistanceXZ`, local `fHeight -> fPickedHeightOffset`; declaration states signed offset; `vPosA -> vPlayerPosition`, `vPosB -> vLadderPosition/vPickedPosition` within the respective queries | Medium; C++ query declarations/definitions and two internal call sites updated; no script binding, overrides or save coupling found |
| R34 / verified | [GameLadder.cpp](game/entities/GameLadder.cpp): entry search moves `mtxPos` upward until collision output matches it; sound setters use `asX` | `mtxPos -> mtxEntryPosition`, `vNewPos -> vResolvedPosition`, `bFound -> bEntryClear`, setter `asX -> asSound`; comments identify upward clearance search and initial position, make blocked-entry feedback TODO actionable | Low; locals/arguments/comments only; search step/limit, walk/swim changes and climb handoff order preserved |
| R35 / verified | [GameVentArea.h](game/entities/GameVentArea.h), [GameVentArea.cpp](game/entities/GameVentArea.cpp): vector `mvMoveSpeed` integrates with timestep; `mfMoveTime` is decremented; positions distinguish floor/character/collision output | `mvMoveSpeed -> mvEntryVelocity`, `mfMoveTime -> mfEntryRemainingSeconds`, `vFloorPos -> vFloorCentre`, `vBodyPos -> vCrouchedCentre`, `vNewPos -> vResolvedCentre`; document attempt latch and conditional carry toward the floor centre | Low; private nonserialized motion state and locals only; persisted `mbPlayerInside` retained; jump/crouch/failure handling, one-second duration and gravity/collision toggles unchanged |
| R36 / verified | [GameSaveArea.cpp](game/entities/GameSaveArea.cpp): loader calls a save area `pLink`; [GameSaveArea.h](game/entities/GameSaveArea.h) hides used-state argument | `pLink -> pSaveArea`, `abX -> abUsed`; roles now match save-area construction and use | Low; local/inline argument only; save-effect calls and persisted `mbHasBeenUsed` unchanged |
| R37 / retained | [GameLink.h](game/entities/GameLink.h) and all four DTOs expose persisted link/sound/message/used-state fields | Retain established class/file/lifecycle names, link fields including fade times and all save macros; split loading-image and locked-interaction comments at the relevant members | Compatibility-sensitive names intentionally retained; link implementation already clear; no pending rename |

Compatibility and caller evidence:

- Read `GameScripts.cpp` link setup, load-image/text overloads, lock setters,
  ladder sound setup and save-area setup plus their registrations. All script
  signatures, strings, wrapper bodies and call expressions remain unchanged.
  Read `Init.cpp` loader registrations and `MapHandler.cpp` saved-area recreation;
  `link`, `ladder`, `vent` and `save` names and load/setup ordering are retained.
- Read climb entry in `PlayerState_Misc.cpp` and its HaptX counterpart before
  documenting entry position; both still consume `mvStartPosition` and the
  unchanged rotation/sound accessors. These supporting reads do not constitute
  a player-state audit. Read `cEffect_SaveEffect::NormalSave` and the used-state
  update before retaining save-area fields and getter/setter contracts.
- Checked the collision resolver's output contract in `PhysicsWorld.cpp`, all
  four DTO/serialization/copy mappings and repository uses of the renamed
  ladder queries. Their only call sites are ladder pick/interact methods.
  Vent motion members are not serialized; the attempt latch is set before
  clearance succeeds and is saved as `mbPlayerInside`, unchanged.

Validation and exit:

- Default Linux build and both CTest suites passed (2/2), including software
  EGL renderer regressions with no skips. The existing `GameScripts.cpp`
  variable-tracking compiler note remains. Token comparison of all eight files
  against this pass's starting sources confirms 69 permitted identifier
  substitutions plus comments/whitespace only. All other tokens, literals and
  serialization/copy macro invocations are unchanged. Local documentation links,
  old-name review and `git diff --check` passed.
- All 32 source files modified before this pass are byte-for-byte preserved.
  No tests were added to mirror renames. Existing suites do not directly cover
  ladder, vent, link or save-area interactions; game smoke/save/reload and manual
  traversal/visual checks were not run. Sanitizer and Windows checks were not
  run for this identifier/comment-only pass; no state/physics refactor is claimed.
- Exit decision: this bounded pass is verified; A3 remains open. Remaining
  entity pairs: `GameCamera`, `GameEntity`, `GameItem`, `GameItemType`, `GameLamp`,
  `GameLever`, `GameObject`, `GameStickArea`, `GameSwingDoor` and `GameWheel`
  (20 headers/sources), plus all player/enemy files. The remaining entity pass is
  recorded below. Other audit phases and ROADMAP validation priorities remain open.

### A3: remaining entities and accumulated diff review (2026-10-18)

Objective: finish the entity subsystem audit, preserve behaviour and external
identifiers, then review the complete uncommitted UI/entity cleanup diff.
Reviewed the remaining 20 headers/sources: `GameCamera`, `GameEntity`, `GameItem`,
`GameItemType`, `GameLamp`, `GameLever`, `GameObject`, `GameStickArea`,
`GameSwingDoor` and `GameWheel` pairs. All 36 entity files are reviewed; 34 have
edits across the three passes, while `GameArea.h` and `GameLink.cpp` are retained
unchanged. No player/enemy-wide audit is claimed.

Reviewed concepts: loader properties and engine-resource association, ownership/
activation/destruction, player pick/interact/examine dispatch, deferred collision
callback removal, scripted movement and attachment, entity variables, body/light/
particle/sound/animation persistence, collision damage, break/disappear/attraction
behaviour, item visibility pulses and inventory actions, lamp fades/materials/
flickering, camera scans/detection/alerts/rays, lever/wheel transitions and return
motion, wheel angle accumulation/controller/joint coupling, door locking/breakage,
and stick-area acceptance/pose interpolation/attachment/detachment/save restoration.

| ID / status | Location and evidence | Implemented mapping / resolution | Risk |
| --- | --- | --- | --- |
| R38 / verified | [GameStickArea.h](game/entities/GameStickArea.h), [GameStickArea.cpp](game/entities/GameStickArea.cpp): detach APIs are misspelled; `GetCallbackFunc` constructs a command, not a function lookup | `GetCanDeatch -> CanDetachBody`, `SetCanDeatch -> SetCanDetach`, `GetCallbackFunc -> BuildBodyCallbackCommand`; update all entity, script-wrapper and normal/HaptX player call sites | Medium; cross-file C++ API rename; script signature/argument `abCanDeatch` and saved `mbCanDeatch` unchanged; interpolation gate retained |
| R39 / verified | Stick-area arguments are opaque; `mfSetMtxTime` measures interpolation progress despite its name; attach comment implies immediate snapping | Setter `asX -> asFunction/asSound/asParticleSystem`, `abX -> abCanDetach/abCheckCenter/abMoveBody/abRotateBody`, `afX -> afDurationSeconds`; `mtxAttachedStart -> m_mtxAttachedStart`, `bCheck -> bPassesBoundsCheck`; document progress completion at >=1 and captured interpolation start pose | Low; private/local/argument names and comments; persisted `mfSetMtxTime` retained; attachment order, callbacks, mass/gravity handling and extrapolation unchanged |
| R40 / verified | [GameLever.h](game/entities/GameLever.h), [GameWheel.h](game/entities/GameWheel.h), [GameSwingDoor.h](game/entities/GameSwingDoor.h): setters use `abX/asX`; wheel limit helper `fLeft` does not identify angle | Lever/wheel `abX -> abStuck`; door `abX -> abLocked`, `asX -> asDescription/asSound`; wheel `fLeft -> fRemainingAngle`; state-transition comments preserve enter-before-leave order; document accumulated angle and signed radians/second, and unrestricted spin direction zero | Low; names/comments only; saved states/angles, joint limits, callback ordering and original connected-offset calculation retained |
| R41 / verified | [GameCamera.h](game/entities/GameCamera.h), [GameCamera.cpp](game/entities/GameCamera.cpp): transient timers and visibility score have generic count names | `mfSpottedPlayerTime -> mfAlertDurationSeconds`, `mfCheckCount -> mfVisibilityCheckDelaySeconds`, `mfAlertCount -> mfAlertRemainingSeconds`, `mlSpottedCount -> mlDetectionScore`; setup `afMinPause/afMaxPause -> afMinPauseSeconds/afMaxPauseSeconds`, `afSpeed -> afRadiansPerSecond`; `abX -> abActive/abAlerted`, ray `pBody -> apBody`, `vPosAdd -> vPlayerSampleOffsets`, `lPosAddNum -> lRayTargetCount`, `lCount -> lBlockedRayCount` | Low; private/local/argument names; saved scan/pause state and `SpottedPlayerTime` asset key retained; comment describes score decay without claiming consecutive detections |
| R42 / verified | [GameItem.h](game/entities/GameItem.h), [GameItem.cpp](game/entities/GameItem.cpp): `IsInView(afMinDist)` rejects distances above the argument; check/flash timers are counts | `afMinDist -> afMaxDistance`, ray declaration `pBody -> apBody`; `mfCheckFlashCount -> mfFlashCheckElapsedSeconds`, `mfCheckFlashMax -> mfFlashCheckIntervalSeconds`, `mfStartFlashCount -> mfFlashEvidenceSeconds`, `mfFlashAlphaAdd -> mfFlashDirection`; correct ray-segment comment | Low; private/argument names; flash direction is a sign, with separate rise/fall rates retained; persisted thresholds/flash flag, ray tests and item pickup order unchanged |
| R43 / verified | [GameLamp.h](game/entities/GameLamp.h), [GameLamp.cpp](game/entities/GameLamp.cpp): `mfAlpha` scales billboard colour while lights fade independently | `mfAlpha -> mfBillboardFade`, `mfTurnOnTime/mfTurnOffTime -> mfTurnOnDurationSeconds/mfTurnOffDurationSeconds`, `alIdx -> alLightIndex`, `abX -> abLit/abFlickering`; correct duplicated update-alpha heading | Low; private/argument names; material swapping, particles, sound, callback/load policy and asset property names unchanged |
| R44 / verified | [GameEntity.h](game/entities/GameEntity.h), [GameEntity.cpp](game/entities/GameEntity.cpp): `SetSoundEntity` destroys a sound; zero-argument `SetCharBody` returns a pointer; movement documentation precedes bone attachment | `SetSoundEntity -> DestroySoundEntity`, getter `SetCharBody() -> GetCharBody()`; move movement contract to `MoveToPosition`, include bodyless mesh movement and clarify successful attachment lifecycle/failure; `vTempBodies -> vSavedBodies`, `lCount -> lSavedParticleIndex`; fix `RemoveCollideScript` declaration `asFunc -> asEntity` | Medium; public C++ API names, but no callers/registrations of the two mislabeled methods; setter overload retained; no ownership, callback or persistence changes |
| R45 / verified | Entity inline setters/accessors hide role or misspell entity argument | `abX -> abActive/abShowOnce/abPause`, body/joint/light/haptic accessor `alNum/alX -> alIndex`, distance `afX -> afDistance`, toughness `alX -> alToughness`, `apEnity -> apEntity`; clarify destructor/collision-body-list comments | Low; argument names/comments only; virtual contracts and saved legacy spelling `mbShowDescritionOnce` retained |
| R46 / verified | [GameObject.h](game/entities/GameObject.h), [GameObject.cpp](game/entities/GameObject.cpp): attraction and duplicate-check delays are counts; disappearance state and destroy-sound member lack clear roles | `mfAttractCount -> mfAttractionCheckDelaySeconds`, `mfCloseToSameCount -> mfDuplicateCheckDelaySeconds`, `mpCurrentAttraction -> mpAttractedEnemy`, disappear `mfTime -> mfRemainingSeconds`, `msDestoySound -> msDestroySound`; remove stale per-body TODO because loop already checks each body | Low; private/transient property names; asset attribute `DestoySound` retained; enemy selection, break/deletion order and thresholds unchanged |
| R47 / verified | [GameItemType.h](game/entities/GameItemType.h), [GameItemType.cpp](game/entities/GameItemType.cpp): actions use count-like `alActionNum`; argument has member prefix; throw/flashlight comments name other actions | `alActionNum -> alActionIndex` in base/overrides/definitions, `GetActions` argument `mbCanBeDropped -> abCanBeDropped`; correct throw/flashlight action labels and painkillers heading | Low; argument names/comments only; action indices, item lifetime, batteries/equipment/notes/collectables and public action strings unchanged |
| R48 / retained | All entity save DTOs/macros, script callbacks/registrations, asset attributes and existing game branches | Retain saved field/class names, including stick-area progress/typos, camera scan state, entity movement/callback/animation data and wheel coupling data; retain clear class/file/lifecycle names and short local math names | Compatibility-sensitive identifiers intentionally retained; no deferred naming finding |

Caller and compatibility evidence:

- Read script setup/lock/stuck/state/speed/connection wrappers, callback command
  construction and registrations in `GameScripts.cpp`; only its C++ call to
  `SetCanDetach` changes. Read all normal/HaptX grab/move/push detach uses before
  replacing the C++ query. These two player files have call-site changes only;
  their wider state-machine audits remain for the next A3 work.
- Checked inventory pickup/action/use/combine dispatch, loader registration in
  `Init.cpp`, map entity lookup/update/destruction and saved recreation in
  `MapHandler.cpp`, as well as enemy character-body setup and object-attraction
  calls. Lifecycle and callback ordering remain intact; no files/build lists,
  script signatures, setting keys, updater names or normal profiles change.
- Read every entity serialization block and save/load/setup implementation.
  Compared all serialization/copy macro invocations and literals exactly.
  Verified timing from timestep use and camera/wheel angular units from update
  arithmetic and script forwarding. Retained original unusual calculations,
  disabled code and error paths rather than mixing behavioural fixes into cleanup.

Validation and accumulated diff review:

- Default Linux build and both CTest suites passed (2/2), including software EGL
  renderer regressions with no skips. Existing nested-comment warnings in unchanged `GameEnemy_Worm.cpp` and the
  `GameScripts.cpp` variable-tracking note remain. Existing suites do not
  directly cover these entity interactions; no tests were added to mirror renames.
- This pass's 20 entity files plus three caller files have 323 permitted identifier
  substitutions, with comments/whitespace only otherwise. All 39 previously
  modified source files remain byte-for-byte intact, including the complete UI
  cleanup and earlier environmental/traversal edits.
- Reviewed the complete uncommitted source diff against `981e71e`, including UI,
  all entity batches and the three supporting callers. Across 62 modified C++
  files, token comparison confirms 914 identifier substitutions; other tokens,
  constants, literals and serialization/copy invocations match. Checked rename
  destinations that already existed: parameter scopes, entity/world sound APIs,
  main-menu difficulty access and the separate inherited widget/list positions
  retain their original bindings. Reviewed changed comments against their code.
- Review corrected the new camera sample-offset name to avoid implying that the
  offsets locate the light ray target, and documented stick progress as complete
  at >=1 rather than suggesting clamping. No remaining behaviour or compatibility
  issues were found in the cleanup diff. Documentation links and diff whitespace
  passed; this source review does not establish correctness of existing code.
- Sanitizers, in-game smoke/save/reload, manual mechanism/camera/item/lamp/audio
  checks, visual captures and Windows checks were not run for this verified
  identifier/comment-only pass. Hardware/visual/gameplay coverage remains limited
  to the evidence recorded in ROADMAP; no new gameplay or visual pass is claimed.
- Exit decision: the full `game/entities/` naming/comment audit is verified and
  complete. A3 remains open for all `game/player/` and `game/enemies/` beyond the
  supporting reads/edits above. A0's wider inventory and A4–A7 remain open. Next
  A3 scope is bounded player interaction/state reviews, followed by enemy states;
  ROADMAP's existing gameplay/renderer validation remains unchanged.

### A3: player and enemy completion record (2026-10-18)

Objective: finish the remaining player interaction/movement/equipment and enemy
state/perception naming audit after `7083979`, retaining executable behaviour
and every script/save/settings identifier. The earlier 36-file entity audit
remains complete; two entity callers follow the player state member rename.

Reviewed scope (all 26 player and 12 enemy headers/sources):

| Files under `game/` | Reviewed responsibility / retained contracts |
| --- | --- |
| `player/Player.h`, `.cpp`, `PlayerState.h`, `PlayerMoveStates.h` | Input dispatch, action/movement transitions, jump/crouch/swim, camera offsets, collision callbacks, global/world saves and statistics |
| `player/PlayerHelper.h`, `.cpp` | Ground/pick/lean rays, head motion, health/oxygen, ringing/deafness, damage/death, flashlight/glowstick/flare, noise/fear/look-at and hiding/proximity feedback |
| `player/PlayerState_Interact.h`, `.cpp`, `PlayerState_InteractHaptX.h`, `.cpp` | Grab/move/push, message return state, body/callback ownership, gravity restoration, throw and rotation paths |
| `player/PlayerState_Misc.h`, `.cpp`, `PlayerState_MiscHaptX.h`, `.cpp` | Normal/interact/item/message states, ladder attachment, signed step travel and top exit |
| `player/PlayerState_Weapon.h`, `.cpp`, `PlayerState_WeaponHaptX.h`, `.cpp` | Throw/melee input, equipment preservation across messages, holstering, movement and haptic attack cooldown/collider lifetime; ordinary weapon header retained unchanged |
| `player/PlayerHands.h`, `.cpp`, `HudModel_Throw.h`, `.cpp`, `HudModel_Weapon.h`, `.cpp` | Equipment entities/effects, camera samples/interpolation, normalized equip/charge/attack progress, hit dispatch and asset properties |
| `player/HapticGameCamera.h`, `.cpp` | Proxy/camera coordinates, frame/hand/render activation, contact effects and world lifecycle |
| `enemies/CharacterMove.h`, `.cpp`, `GameEnemy.h`, `.cpp` | Path clearance/steering, squared-distance history, stuck/turn/brake logic, perception, animation completion, state transitions, patrols, triggers/callbacks and persistence |
| `enemies/GameEnemy_Dog.h`, `.cpp`, `GameEnemy_Infected.h`, `.cpp` | Idle/patrol/attention/investigate/move-to/hunt/attack/break-door/flee/backup/knockdown/dead/eat states; both Dog AI modes, remembered observations, Infected weapon/flashlight/throat grab/healing and saves |
| `enemies/GameEnemy_Spider.h`, `.cpp`, `GameEnemy_Worm.h`, `.cpp` | Idle/hunt/attack/flee/knockdown/death as applicable, flashlight response, attack/sound delays, Worm tail smoothing/mesh callbacks and movement sound |

Findings and resolution (source changes are in this working tree):

| ID / status | Evidence | Exact mapping / resolution | Dependencies and risk |
| --- | --- | --- | --- |
| R49 / verified | [Player.h](game/player/Player.h), resource setters use `afX`; `RemoveCollideScript` takes an entity name, not a function | `afX -> afHealth/afPower/afOxygen/afMass` in the corresponding setters, `afX -> afAmount` in adders; height/head setters use `afHeightOffset/afSizeMul/afSpeedMul`; `asFunc -> asEntity` in removal declaration | Low; declaration/definition arguments only; script wrappers and saved fields unchanged |
| R50 / verified | [PlayerState.h](game/player/PlayerState.h), previous-state spelling; [PlayerHelper.h](game/player/PlayerHelper.h), death height getter typo | `mPreviuosState -> mPreviousState`, `GetHeighAdd -> GetHeightAdd`; update `GameEntity.cpp`, `GameStickArea.cpp` and player camera caller | Medium; cross-file C++ symbols; no registration or serialization use; enter/leave and detach ordering unchanged |
| R51 / verified | Both interaction-state pairs obscure member ownership and collision backoff versus input grace | `bPausedGravity -> mbPausedGravity`, `mlBackCount -> mlBackoffUpdatesRemaining`, `mlMoveCount -> mlMoveInputGraceCount`; correct restore-gravity comment | Low; transient state/callback members; grace still decrements in yaw/pitch handlers, backoff in update; no cadence change |
| R52 / verified | Both misc-state pairs call ladder travel a count and velocities additions | `mfTimeCount -> mfAttachRemainingSeconds`, `mfStepCount -> mfStepTravelDistance`, `mfLeaveAtTopCount -> mfTopExitRemainingSeconds`, `mvPosAdd -> mvAttachVelocity`, `mvRotAdd -> mvAttachAngularVelocity` | Low; transient ladder state; signed distance, attachment collision policy and double top-exit decrement while grounded preserved |
| R53 / verified | [PlayerHands.h](game/player/PlayerHands.h), [HudModel_Weapon.h](game/player/HudModel_Weapon.h): base and derived `mfTime` are distinct normalized values | Base `mfTime -> mfEquipProgress`; melee `mfTime -> mfAttackProgress`, `mfMoveSpeed -> mfPoseProgressRate`, `mfTimeOfAttack -> mfDamageProgress`; throw `mfChargeCount -> mfChargeProgress`, `mfChargeTime -> mfChargeDurationSeconds`; `LoadExtraEntites -> LoadExtraEntities`, `UpdatePrevPostions -> UpdatePreviousCameraSamples`, attack argument `alX -> alAttackIndex` | Medium; base/override/caller names and hidden members reviewed; document negative equip reload offset without interpreting it as seconds; asset keys and both original member bindings preserved |
| R54 / verified | [PlayerHelper.cpp](game/player/PlayerHelper.cpp), unrelated timers share `mfTimeCount`; hiding and flashlight counters lack direction/units | Health `mfTimeCount -> mfHeartbeatDelaySeconds`, ear ringing `mfTimeCount -> mfRemainingSeconds`, noise `mfTimeCount -> mfNoiseRefreshDelaySeconds`; `mfRayCastTime -> mfEnemyRayElapsedSeconds`, `mfHiddenCount -> mfHideElapsedSeconds`, `mfHiddenOnCount -> mfHiddenElapsedSeconds`, `mfCheckEnemyCloseCount -> mfEnemyProximityCheckDelaySeconds` | Low; helper-local transient members; original pulse/ray/hiding/audio timing and shader feedback preserved |
| R55 / verified | [HapticGameCamera.h](game/player/HapticGameCamera.h), proxy typo; opaque booleans in player/helper/camera declarations | `mvPrevPrxoyPos -> mvPrevProxyPos`; `abX -> abActive/abDisabled/abVisible/abUseFrame` in matching helper/camera methods; player flags use `abEnabled/abNoFallDamage/abUnderWater/abFlash/abLanded/abDown`; HaptX `CheckAttack` declaration `mtxTransform -> a_mtxTransform` | Low; private member and arguments only; all activation side effects and haptic resource lifetimes unchanged |
| R56 / verified | All four enemy hunt pairs label an interval frequency and delays counts | `mfUpdatePathCount -> mfPathUpdateDelaySeconds`, `mfUpdateFreq -> mfPathUpdateIntervalSeconds`, `mfLostPlayerCount -> mfLostPlayerRemainingSeconds`, `mfMaxLostPlayerCount -> mfLostPlayerDurationSeconds`; Dog/Infected `mfHearSoundCount -> mfNoiseRetargetDelaySeconds`; Infected `mfAttackPauseCount -> mfAttackPauseRemainingSeconds`, `mfHuntSoundCount -> mfHuntSoundDelaySeconds`; Worm `mfAttackCount -> mfAttackCooldownSeconds`, `mfAttackSoundCount -> mfAttackSoundCooldownSeconds`, `mfSoundCount -> mfHuntSoundDelaySeconds` | Low; state members, plus existing Dog fixture references; original/improved branches, lost-target policies, random calls and attack/sound thresholds unchanged |
| R57 / verified | [GameEnemy.h](game/enemies/GameEnemy.h), player-check rate is an interval; pose counter interpolates rotations | `mfCheckForPlayerRate -> mfPlayerCheckIntervalSeconds`, `mfCheckForPlayerCount -> mfPlayerCheckElapsedSeconds`, `mfPoseCount -> mfGroundPoseProgress`, `mbOverideMoveState/abOverideMoveState -> mbOverrideMoveState/abOverrideMoveState`, `mfMoveToLideLimit -> mfMoveToIdleLimit`; damage args `afX -> afDamage`, activation `abX -> abActive`, previous state `alX -> alStateId`, patrol index `alX -> alNodeIndex`, patrol time `afTime -> afWaitSeconds`, wait setters/adders `afX -> afDurationSeconds/afElapsedSeconds/afSeconds`, flags `abX -> abStop/abAttracted/abUsesTriggers` | Low; base members/arguments and Dog fixture references; saved patrol `mfWaitTime`, callbacks, state IDs and animation completion ordering retained |
| R58 / verified | [CharacterMove.cpp](game/enemies/CharacterMove.cpp), distance history is squared and `mfNodeDistAvg` is a sum | `mlstNodeDistances -> mlstNodeSquaredDistances`, `fNodeDist -> fNodeDistanceSqr`, `mfNodeDistAvg -> mfNodeDistanceChangeSum`; document differences from oldest sample; clarify braking threshold comment; setter args `abX -> abEnabled`, `afX -> afRadiansPerSecond/afTurnGain/afRadians/afBrakeGain/afMass`, `alX -> alToughness` | Low; local/private names and inline arguments; no arithmetic correction, threshold/unit conversion or path policy change |
| R59 / verified | Movement/weapon paths contain timer/forward typos and stale running prohibitions | Swim `mfSwimSoundCount -> mfSwimSoundDelaySeconds`, jump `mfDefaultFowardSpeed -> mfDefaultForwardSpeed`; HaptX melee `mfAttackCount -> mfAttackCooldownSeconds`; remove weapon-running comments contradicted by run handlers | Low; transient names/comments; movement state restore, collider lifetime and attack cadence preserved |
| R60 / retained | Player/global/world saves, enemy/patrol/Infected DTOs, script/callback registrations and asset/config properties | Retain all serialized class/field names and macros, script signatures, updater/asset/settings/diagnostic strings, lifecycle/state names and clear class/file names; retain scoped math/loop names and unrelated same-spelled members | Compatibility-sensitive identifiers checked and intentionally retained; helper groupings need no file/class split for this naming pass; no deferred naming finding |

Caller/compatibility evidence and validation:

- Checked script wrappers/registrations for player resources, collision callback
  removal, patrols, enemy callbacks and Infected equipment; global player saves
  and enemy/patrol/Infected serialization/load/save paths retain identifiers.
  Read the entity detach callers and existing Dog fixtures before propagating
  names. `docs/dog-foundations.md` describes unchanged behaviour and needs no
  API documentation changes. No files/build lists or normal profiles change.
- Compared the complete source diff against `7083979`: 40 changed C++ files,
  577 permitted identifier substitutions, other tokens/literals unchanged;
  serialization/copy macro lines match exactly. Reviewed the three preexisting
  destination names (`afSpeedMul`, `asEntity`, `afDamage`) in their parameter
  scopes. Checked the separate base equip and derived attack members explicitly.
  Old-name matches are disabled examples or unrelated members; disabled examples
  touched by this pass use the new names. No tests added to mirror renames.
- Validation: default Linux build and both CTest suites passed (2/2), including
  software EGL renderer regressions with no skips. HaptX sources and the updated
  Dog fixtures compile. Documentation links and diff whitespace passed. Existing
  ButtonHandler missing-initializer warnings, Worm nested-comment warnings and
  the GameScripts variable-tracking note remain. In-game smoke/save/reload, Dog runtime checks,
  sanitizers, manual HaptX/gameplay/audio, visual captures and Windows checks
  are not run for this identifier/comment-only pass. Existing automated suites
  do not establish these interaction/encounter behaviours in game.
- Exit decision: A3 naming/comment audit verified; all 74 headers/sources (36 entities,
  26 player, 12 enemies) reviewed; no remaining A3 naming scope. A0's wider
  inventory and A4–A7 remain open; next scope is A4 core and persistence.
  ROADMAP's outstanding gameplay/renderer validation remains unchanged.

### A4: core and persistence (2026-10-19)

Reviewed the 34 files in `game/core/`: map loading/cache/timers and saved-world
restoration; SaveHandler/SaveTypes/GameTypes declarations and serialization;
Init startup, per-game paths/config, updater registration and teardown; script
wrappers/registrations and counting stubs; input/cursor helpers; graphics/lamp
helpers; smoke, map, Dog and reference/effects diagnostics. Retained the
precompiled/aggregate headers, startup globals, product/version history and
clear class/file names. No structural extraction is required by these findings.

| ID / status | Location and evidence | Resolution / dependencies / risk |
| --- | --- | --- |
| R61 / verified | `MapHandler.h/.cpp`: flash `afAddTime`, `afNegTime`, `mfNegTime` describe fade durations; `PreUpdate` receives elapsed game time | Rename to `afFadeInSeconds`, `afFadeOutSeconds`, `mfFadeOutSeconds`, `afSecondsSinceVisit`. Low; arguments/private transient member; FadeTo and pre-update arithmetic unchanged |
| R62 / verified | `RefView.h/.cpp`: `mfWaitTime` accumulates map wait time; `abX` opens the panel | `mfMapWaitElapsedSeconds`, `abOpen`; correct obsolete shadow-volume comment. Low; no state or rendering change |
| R63 / verified | `Init.h`, `LampAppearanceController.h`, `SaveHandler.h`, `DogTestMap.cpp`: opaque or mismatched arguments | `abX -> abEnabled/abStill`; BeamExists declaration `apPS -> apBeam`; ResetStation/PlayLure definitions match `alStation`, `abMovePlayer`, `alLure`. Low; parameter-only edits |
| R64 / retained | `cGameTimer::mfTime`, loaded-map/global save fields, `bKillMe`, serialization class names, script signatures, settings/profile/updater strings | Preserve compatibility identifiers; document timer remaining seconds at its declaration. Do not rename script APIs, legacy product metadata or save DTOs merely to fix spelling |
| R65 / verified | SaveHandler comments call saving music loading and misspell deleting saved records | Correct adjacent comments; destructor and serialization operations retained. Low |

Validation: default Linux build and both CTest suites passed (2/2), including
software EGL without skips. Token comparison confirms only identifier changes
outside comments; literals, operators, control flow and persistence macro lines
are unchanged. No file/build changes. In-game saves/smoke, Dog runtime checks,
manual play, sanitizers, captures and Windows were not run for this naming and
comment pass. Existing gameplay/renderer validation remains in ROADMAP.

Remaining A4 naming scope: none. Next: shared-engine APIs and shader contracts.

## Structural refactoring

Start only when a concrete problem remains after names and formatting are clear:
duplicated policy, mixed responsibilities, implicit ownership or an oversized
method with identifiable stages.

- Extract a helper or responsibility with explicit inputs and outputs.
- Keep construction/destruction order, callbacks and update registration intact.
- Keep interfaces stable where practical; propagate necessary interface changes
  through all implementations and callers in the same batch.
- Avoid introducing a new abstraction, class hierarchy or moving whole folders
  unless the responsibility boundary justifies it.
- Review memory lifetime, error paths and state restoration explicitly.

## Validation by change

| Change | Required evidence |
| --- | --- |
| Documentation only | Local links/paths resolve; `git diff --check` passes, accounting for existing Windows CRLF files |
| Source comments, C++ formatting or naming | Default build and relevant existing regressions; verify comment edits leave code intact, identifier/include changes are complete and public strings are unchanged |
| Ownership, parsing, serialization, physics or animation refactor | Default checks plus ASan/UBSan build and relevant tests; inspect save compatibility and run applicable isolated game checks |
| Renderer API, resources or state | Engine and renderer CTest suites; GL 3.3 extension and GL 4.3 core paths; ASan/UBSan for ownership changes; before/after captures when rendering paths are affected |
| Gameplay, player or enemy refactor | Relevant regressions and isolated ASan game smoke; Dog checks for Dog AI/animation; manual comparison for changed mechanisms or behaviour |
| Build/file/platform renames | Verify CMake, includes, launchers and tooling; Windows build/smoke when Windows paths are affected |

Choose checks by what the change affects. For verified identifier/comment-only
changes, use the default build, relevant existing regressions, compatibility
searches and token/diff review. Changing a physics or audio variable's name
alone does not require physics smoke tests, listening checks or sanitizers.
Use broader checks when executable behaviour, ownership, parsing, serialization,
physics, animation, rendering or platform/build paths change, as specified
above and in AGENTS. Keep actual validation limits in the audit record; final
reports should highlight relevant gaps without repeating a list of every
unrelated check or an unchanged commit status each turn.

Use the commands and isolated runners in [AGENTS.md](AGENTS.md) and
[README.md](README.md). Preserve normal profiles and installed assets. Record
sanitizer leak-detection settings, skipped EGL tests and unavailable game,
visual or Windows checks explicitly. A successful build alone does not prove
gameplay or rendering preservation.

## First batch and completion checklist

- [x] Inspect `game/systems/FadeHandler.h` and `.cpp`, their callers and tests.
- [x] Confirm or revise R01–R03; inspect function/class/file names and comments.
- [x] Record caller/compatibility evidence and retain clear names explicitly.
- [x] Choose a naming or comments/formatting batch and list its scope.
- [x] Apply it without changing public strings, timing or behaviour.
- [x] Review and validate; record results.
- [x] Include the pilot and systems pass in the requested `naming-cleanup` commit.

For each later batch, record scope, rename mapping or extraction, compatibility
checks, tests run and remaining validation. A batch is complete when all callers
are consistent, the diff matches its stated purpose and applicable checks have
passed or their limitations are documented. There is no requirement to rewrite
every folder or rename every legacy identifier.

## Audit completion

The audit is complete when A0–A7 have recorded reviewed scope, every finding
is verified, retained or explicitly deferred with a reason, and no unreviewed
scope is silently treated as clean. Implementation and validation completion
must be reported separately when checks are unavailable. Deferred structural
work and validation gaps stay linked here or in the relevant current guide;
project priorities remain in ROADMAP.

Review completion by sampling navigation from a caller to its declaration and
implementation: do the names explain the operation, can the responsible file
be found, and are essential contracts visible without a long search? Check
that comments add verified context and old-name matches are intentional.
This is a qualitative review, not a comment coverage percentage, identifier
length rule or requirement to rename every legacy symbol.
