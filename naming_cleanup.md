# Code readability audit and cleanup plan

Status: audit started on 2026-10-18. The FadeHandler pilot, RadioHandler
positional-distance batch and systems pass are verified by the default Linux
build and both CTest suites. All 20 `game/systems/` headers/sources have now
been reviewed for naming/comments; A2 remains open for `game/ui/`. The wider
inventory and later phases remain open.

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

The next pass covers `game/ui/` under A2. The systems pass, including
RadioHandler's queue, callback and subtitle naming, is recorded below. Keep
findings grouped by responsibility and risk without turning each group into
a separate user turn.

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
| A0: inventory and criteria | Apply the standards above; list reviewed and remaining areas; expand the seed findings with evidence, dependencies and risk | Initial work queue has concrete bounded batches and required checks; uninspected areas are explicit | Started: FadeHandler seed inspection only |
| A1: pilot | Audit `game/systems/FadeHandler.h` / `.cpp`, direct callers and compatibility sites; resolve R01–R03 or record reasons to retain them; inspect comments separately | First naming batch validated; comment decisions recorded; actual results establish the pattern for later batches | Verified |
| A2: systems and UI | Review remaining `game/systems/`, then `game/ui/`; inspect a contained system such as RadioHandler and one screen interaction at a time | Reviewed scope recorded; actionable findings resolved or deferred with reasons; timing, coordinates, callbacks and resource roles remain clear | Systems naming/comment review complete; UI open |
| A3: entities, player and enemies | Review `game/entities/`, `game/player/` and `game/enemies/` by mechanism/state; audit names and transition comments | State/callback/physics findings dispositioned; save/script strings checked; applicable smoke, Dog, sanitizer and manual results recorded | Open |
| A4: core and persistence | Review `game/core/` map/save/script helpers, then bounded sections of `cInit`; document registration and lifetime contracts where needed | Compatibility-sensitive identifiers mapped and retained where required; game-selection, save and initialization contracts checked | Open |
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

Coverage includes all 20 headers/sources in `game/systems/`, with reviewed
concepts, callers and retained names recorded below. `game/ui/` remains open;
reading an inventory/subtitle caller does not count as reviewing that UI file.

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
- Systems naming/comment review is complete; A2 remains open for all 26 UI
  headers/sources. Later phases and gameplay/renderer validation in ROADMAP
  remain open.

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
