# Frame-time work (`fix/opti`)

Plan and progress for the `fix/opti` branch: what eats frame time on the
Overture maps, and the rendering, shader and CPU changes to reduce it. Tick
items as they land and record each one's before/after numbers here; the
commit message keeps the implementation detail.

Status key: `[ ]` open, `[~]` in progress, `[x]` done (with measurements).

## Benchmark: one viewpoint per feature

`tools/framebench.py` (see its header and README) measures each rendering
feature at the viewpoint where the all-maps survey below found it costs the
most. A full run takes about 110 s; `--features ao,lights` only loads the
maps those need. `level01_20_base_entrance` is left out although it tops
some features: its scripted lights change from run to run.

Per viewpoint, GPU times are taken with frames drawn as fast as they can be
(`option pace 0`, one logic step per frame, so the frames are the same every
run) and are medians over 300 frames. CPU times are taken at 60 Hz (the
main thread then does not wait for the GPU in the driver), from 1 kHz
stack samples. Same settings as the survey: 1920×1080, reference-capture
graphics, vsync off.

Baseline on the RTX 2060, two runs of the same build (ms; the feature's
own time, not the whole frame):

| Feature | What | Viewpoint | Run 1 / run 2 | Spread |
|---|---|---|---|---|
| `ao` | ambient occlusion | `level01_07_lead_mine_shafts_a` view02 | 1.31 / 1.32 | 1% |
| `lights` | light passes without their shadow maps | `level00_01_boat_cabin` view01 | 2.01 / 2.05 | 2% |
| `shadows` | shadow-map rendering (GPU) | `level01_07_lead_mine_shafts_c` view02 | 1.11 / 1.07 | 4% |
| `shadowcpu` | shadow casters and maps (CPU) | `level01_07_lead_mine_shafts_c` view02 | 1.23 / 1.46 | 19% |
| `transparent` | transparent objects, halos, particles | `level01_01_outside` view03 | 1.17 / 1.16 | 0% |
| `z` | depth and ambient pass | `level01_04_old_storage` view01 | 0.56 / 0.77 | 38% |
| `bloom` | bloom | `level01_09_generator_room` view02 | 0.37 / 0.37 | 0% |
| `smaa` | SMAA | `level01_14_refinery` view03 | 0.37 / 0.37 | 1% |
| `submit` | draw submission (CPU) | `level01_15_icecave` view03 | 1.32 / 1.22 | 7% |
| `visibility` | render list and visibility (CPU) | `level01_14_refinery` view01 | 0.81 / 0.79 | 2% |
| `physics` | physics (CPU) | `level01_13_b_iron_storage_tunnels` view02 | 2.69 / 2.75 | 2% |

Changes within the spread are noise: run `z` and `shadowcpu` several times
before trusting a difference. The whole-frame CPU and GPU totals per
viewpoint move by 10–18% between runs; judge a change by its feature's
number.

## Survey baseline (2026-11-11)

Measured on Linux, RTX 2060 (driver 620.70.06), 1920×1080, the reference
capture settings of `tools/levelview.py` (4× MSAA, SMAA, AO, bloom, all
shadows, refractions) with vsync off. All 20 Overture maps, 86 viewpoints
from `tools/refviews.txt`, 300 frames each after 120 frames of settling.
Black Plague and Requiem were not profiled.

How it was measured (temporary tooling, not in the repository yet — see
item 0):

- An `LD_PRELOAD` library sampled the main thread's stack at 1 kHz and
  timestamped every `SDL_GL_SwapWindow`; samples were symbolised with
  `addr2line`.
- `OPENHPL_GPU_LOG` recorded the engine's per-section GPU timestamps.
- A paced pass (60 Hz fixed step) gave GPU sections and CPU busy time; an
  uncapped pass gave achieved frame times. The paced GPU numbers are high:
  at 60 Hz the GPU is lightly loaded and lowers its clocks (AO measured
  1.8 ms there against 1.3 ms at full load in the benchmark above). The
  ranking of features and viewpoints still holds; use the benchmark's
  numbers for before/after. The uncapped pass was
  only run on `level01_05_office` (stopped to spare the GPU), and the GPU log
  drops frames there because queries are not ready when the GPU lags.

### Summary

No viewpoint is near the 60 FPS budget on this GPU:

| | Mean | Worst viewpoint |
|---|---|---|
| Main-thread CPU busy per frame | 4.6 ms | 7.6 ms (`level01_13_b` view03) |
| GPU busy per frame (excluding Present) | 5.0 ms | 6.0 ms; p99 8.4 ms (`level01_20` view01) |
| Draw calls | 247 | 841 |
| Triangles | 38k | 159k |

### GPU sections (mean / max over viewpoints, ms)

| Section | Mean | Max |
|---|---|---|
| World / Ambient occlusion | 1.83 | 2.38 |
| — Search | 1.04 | 1.35 |
| — Blend | 0.30 | 0.39 |
| — Depth tiles | 0.25 | 0.48 |
| — Filter | 0.08 | 0.25 |
| World / Lights (includes shadow maps) | 1.37 | 2.99 |
| — Shadow maps | 0.15 | 1.33 |
| Post effects / Bloom | 0.50 | 0.58 |
| Scene output / SMAA | 0.40 | 0.74 |
| World / Z | 0.39 | 0.78 |
| World / Transparent | 0.16 | 1.26 |
| Present | 11.7 | — (includes idle waiting at 60 Hz; real cost unknown) |

### Main-thread CPU (ms per frame, all frames)

| Work | ms |
|---|---|
| Render world (draw submission, lights, AO passes) | 1.04 |
| Physics (Newton; leaf hotspot `dgBroadPhaseCell::UpdateAutoPair`) | 0.81 |
| Scene render, other | 0.61 |
| Game update, other | 0.54 |
| Render list / visibility (`cPortalContainer::GetVisible`) | 0.40 |
| Post effects | 0.37 |
| Shadow maps | 0.23 |
| Swap | 0.22 |

Heaviest CPU viewpoints: `level01_13_b_iron_storage_tunnels`,
`level01_13_c_iron_base_tunnels`, `level01_14_refinery`,
`level01_20_base_entrance` (7–7.6 ms, 600–840 draw calls).

### Open observations

- Uncapped on `level01_05_office`, the main thread spends nearly all its
  time blocked in the driver: the GPU limits the frame rate there.
- Same run: stalls of 0.8–1.6 s about once a second, blocked in
  `cGLRenderTargets::BlitScene` / `cGLFinalPasses::PresentFrame`. The run
  used a hidden Wayland window; not confirmed in a visible window. Check
  with item 0's visible-window pass, or `OPENHPL_DEBUG=1 ./run.sh`, F3,
  "Limit to 60 FPS" off.

## Done

- [x] **Busy wait with "Limit to 60 FPS" (CPU).** Between 60 Hz logic steps
  `cGame::Run` spun without sleeping, keeping a core at 100%. It now rests
  until the next step is due (`iLowLevelSystem::SleepPrecise`: sleep, then
  wait out the last 2 ms; `cLogicTimer::GetMillisecsToUpdate`). The paced
  fixed step of live sessions uses the same sleep. Fixed-step runs (smoke,
  captures) and uncapped play are unaffected.
  - Main-thread CPU on `level01_05_office`, 60 FPS limit, vsync off,
    10 s: 100% → 24% (two runs).
  - Frame interval p50 unchanged (16.67 ms); p99 17.2 → ~18.9 ms, one
    44 ms outlier. A 6 ms spin margin did not reduce it, so late wake-ups
    are not the cause; probably CPU clocks dropping between frames
    (unconfirmed). Not checked with vsync on.
  - Validation: build, CTest 3/3, `OPENHPL_SMOKE=office` 0 failures. No
    sanitizer run, no manual play.

- [x] **Static pairs in Newton's broad phase (CPU).** `level01_13_B` has
  1163 bodies, 1089 of them static (no mass), none moving once settled; the
  broad phase still overlap-tested every static/static pair in active cells,
  which `dgCollidingPairCollector::AddPair` then dropped. `UpdateAutoPair`
  and `UpdatePairs` (vendor/newton/physics/dgBroadPhaseCollision.cpp) now
  skip such pairs before the test. The live `status` command reports
  `bodies N static N moving N`.
  - `physics` feature: 2.54–2.75 ms → 1.88–2.42 ms (three runs each; about
    −20%).
  - Same simulation: `OPENHPL_PHYSLOG` of a smoke run over `13_B`,
    refinery and testmap is byte-identical before and after (13523 lines).
  - Validation: CTest 3/3 (release and ASan); ASan smoke on office,
    refinery, testmap, `13_B`, `13_C` with 0 failures. The only sanitizer
    report is UBSan's null `memcpy` argument in vendored AngelScript
    (`as_string.cpp:187`), not from this change.

## Plan

### 0. Measurement tool

- [x] `tools/framebench.py` with `tools/framebench_sampler.c`: one viewpoint
  per feature (`--survey` for every viewpoint), GPU sections at full GPU
  load, CPU at 60 Hz, `show` and `compare`. Supporting changes: the live
  command `option pace 0|1`; the GPU profiler keeps 8 frames in flight
  instead of 4, and with `OPENHPL_GPU_LOG` waits for a frame's results
  instead of skipping it; `tools/levelview.py` passes `OPENHPL_GPU_LOG`
  through and notices a crashed game it started itself. The sampler takes
  stacks only while a viewpoint is measured and drops a sample whose stack
  walk faults (an earlier version crashed the game during a map load).
- [ ] A visible-window uncapped pass to settle the 1-second stalls above.
- [ ] Profile Black Plague and Requiem maps (start positions where there are
  no viewpoints).

### Rendering (GPU)

- [ ] **1. Ambient occlusion (1.8 ms).** Search at half resolution with a
  depth-aware upsample; fold the 0.3 ms blend into a following full-screen
  pass. Target under 1 ms. Supersedes the ROADMAP item "Measure AO after
  its existing optimization pass".
- [ ] **2. Lights (1.4 ms, up to 3.0 ms).** Optional depth-bounds test
  (`GL_EXT_depth_bounds_test`) to skip pixels outside a light's range,
  keeping the scissor rectangle as the supported path. Larger option: up to
  2–4 lights per object per draw instead of one pass per light; needs
  careful image comparison.
- [ ] **3. Smaller passes.** Bloom (0.5 ms): drop or cheapen the top
  half-size level. Shadow-map spikes (up to 1.3 ms): check cache reuse (see
  [shadow-cache.md](shadow-cache.md)). Split Present's real GPU cost from
  its idle time before working on it.

### Shaders

- [ ] **4. `hpl1_light.fragment`.** Skip the shadow lookup where the surface
  faces away from the light and has no highlight; today it is skipped only
  where attenuation is 0. Each skip saves a blocker search and PCF.
- [ ] **5. `hpl1_shadow.glsl`.** `textureGather` in the blocker search;
  fewer samples for distant or small lights at lower shader quality.

### CPU

- [ ] **6. Draw submission (~2 ms).** Per-draw state comparisons, uniform
  setup and `PrepareDraw` repeat for every light pass; prepare each
  object's draw state once per frame and reuse it.
- [~] **7. Physics (0.8 ms, 1.1 ms in slow frames).** Resting bodies do
  sleep (`status` shows 0 moving after settling); static pairs are skipped
  (Done). Remaining: the per-step walk over all bodies in
  `iPhysicsWorld::StoreInterpolation` (0.3 ms on `13_B`, mostly static
  bodies) and the broad phase's cell sorting. Behaviour must not change:
  compare `OPENHPL_PHYSLOG` before and after.
- [ ] **8. Visibility (0.4–0.6 ms).** Keep portal visibility and light
  shadow-caster results while the camera and lights have not moved.
- [ ] **9. Draw-heavy maps.** Find what makes 600–840 draw calls on the
  iron-tunnel, refinery and base-entrance maps before choosing batching or
  instancing.

## Validation for each item

Per [AGENTS.md](../AGENTS.md): renderer and shader items need
`engine_regressions` and `renderer_regressions`, both the GL 3.3 extension
and GL 4.3 core paths, and `tools/levelview.py capture`/`compare` before and
after (plus `effects`, and MSAA/scaled checks where touched). Physics and
CPU items need the ASan smoke test. Record the item-0 numbers before and
after each change here, and keep code implemented, tests passed, visual
checks, manual play and measured performance apart.
