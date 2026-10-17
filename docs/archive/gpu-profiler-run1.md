# GPU profiler, run 1

> Archived on 2026-10-18. This is a historical snapshot, not current
> status or agent instructions. See the [current roadmap](../../ROADMAP.md)
> and [archive index](README.md) for context. Paths under old build
> directories and `/tmp` may no longer exist.

2026-10-02, branch `shading` (742dce9 plus uncommitted work).

Ambient occlusion is the biggest GPU cost by a wide margin: about 3.5 ms, 41%
of the frame. Soft shadows and multisampling follow at about 1.2 ms each, and
everything else is small.

## Setup

- NVIDIA GeForce RTX 2060, driver 615.71.09, 2560x1440.
- The user's own `settings.cfg`: 2x multisampling, SMAA, ambient occlusion
  (3 slices, 6 steps, full rate), shadow maps 1024 with the soft-by-distance
  filter (16 samples, 12 blocker samples), soft bloom, linear lighting,
  improved shading, flashlight, lamps, halos, shafts and particles, grain,
  16x anisotropic filtering. Motion blur off.
- The game ran headless (gamescope) through the reference-shot run, which
  visits the hand-picked viewpoints of `tools/refviews.txt`, without HUD.
- Times are the engine's GPU time stamps per section. A view's time is the
  median of the 16 frames before its shot; the frame time is the sum of the
  top-level sections.
- Run-to-run noise is about 0.15 ms. Other desktop programs share the GPU.

## Time per render pass

All maps, 83 views. The frame averages 7.9 ms (median 7.8, worst 16.5).

| Pass | Mean ms | Worst ms | Notes |
|---|---|---|---|
| Ambient occlusion | 3.63 | 5.72 | Nearly the same in every view |
| – search | 1.99 | 2.26 | |
| – depth tiles | 0.61 | 0.61 | |
| – filter | 0.43 | 1.21 | |
| – blend | 0.19 | 0.26 | |
| Lights (with shadow sampling) | 1.47 | 3.72 | Varies most with the scene, 0 to 3.7 |
| – shadow map drawing | 0.19 | 0.91 | |
| Post effects | 0.98 | 3.37 | |
| – bloom | 0.59 | 0.72 | |
| – depth of field | 0.12 | 1.67 | Only where the blur is active |
| Scene output | 0.68 | 1.21 | |
| – SMAA | 0.49 | 1.02 | |
| Depth pre-pass | 0.40 | 0.65 | |
| Transparent (halos, shafts, particles) | 0.34 | 2.70 | Median 0.14; high on the outside map and the ice cave |
| Present | 0.25 | 0.42 | |
| Fog | 0.04 | 0.17 | |

Heaviest views:

| View | Frame ms |
|---|---|
| level00_01_boat_cabin view01 (intro blur at full strength) | 16.5 |
| level00_01_boat_cabin view02 | 11.1 |
| level00_01_boat_cabin view03 | 11.0 |
| level01_02_mine_entrance view01 | 10.6 |
| level01_02_mine_entrance view02 | 10.0 |
| level01_13_a_iron_mine_shafts view01 | 9.9 |
| level01_01_outside view03 | 9.7 |

The lightest views are about 5.7 ms: with no light on screen, ambient
occlusion and the fixed full-screen passes are all that is left.

## Cost of each effect

One run per effect switched off, the other settings unchanged, over 37 views
in 10 maps (boat cabin, outside, mine entrance, office, iron mine shafts A,
refinery, ice cave, new storage room, lead mine shafts B, mining room). The
reference frame there averages 8.54 ms. "Saves" is the reference time minus
the time without the effect, averaged over the views.

| Effect switched off | Saves ms | Share of frame | Most in one view, ms |
|---|---|---|---|
| Ambient occlusion | 3.51 | 41% | 4.23 |
| Shadows (all) | 1.18 | 14% | 2.70 |
| Multisampling (2x) | 1.14 | 13% | 1.82 |
| SMAA | 0.45 | 5% | 1.11 |
| Improved shading | 0.26 | 3% | 1.18 |
| Bloom | 0.25 | 3% | 1.25 |
| Linear lighting | 0.23 | 3% | 1.06 |
| Depth of field | 0.18 | 2% | 0.96 |
| 16x anisotropic filtering | 0.16 | 2% | 0.70 |
| Shafts | 0.02 | 0% | 1.09 |
| Particles | 0.01 | 0% | 1.24 |
| Grain | −0.02 | 0% | 0.67 |
| Improved flashlight | −0.05 | −1% | 0.78 |
| Halos | −0.07 | −1% | 0.47 |
| Improved lamps | −0.35 | −4% | 0.70 |

Cheaper settings of the two largest:

| Change | Saves ms | Share of frame |
|---|---|---|
| Ambient occlusion at half rate | 0.87 | 10% |
| Hard shadow filter instead of soft by distance | 0.98 | 11% |

Two more reference runs came out 0.06 and 0.13 ms slower than the first, so
rows of 0.25 ms and below are only roughly ordered.

## Notes

- Ambient occlusion costs nearly the same in every view.
- Almost all of the shadows' cost is the soft filtering: the hard filter
  saves 0.98 of the 1.18 ms, drawing the shadow maps is about 0.2 ms.
- About 0.7 ms of multisampling's cost lands inside the ambient occlusion
  pass, so the two overlap.
- Depth of field costs nothing in most views (median 0) and shows only
  where the blur is active.
- The frame is 0.35 ms cheaper with improved lamps on; a repeat run gave the
  same sign (−0.49 the first time).
- Shafts and particles average zero but cost about 1 ms in single views
  (boat cabin, outside).
- The first shafts and particles runs came out about 1 ms slow in passes
  they have no part in, and were fine when repeated: something else was
  using the GPU. The table has the repeated runs.

## How to repeat

`OVERTURE_GPU_LOG=<file>` makes the GPU profiler write every frame's section
times to a file, one line a frame. The scripts are in `build/gpu/` (not in
git):

- `gpurun.py NAME MAPS [Key=Value ...]`: a headless reference-shot run with
  the user's settings plus overrides of the Graphics section.
- `analyse.py RUN [views]`: time per section, and per view.
- `sweep.sh`, then `sweep_report.py`: one run per effect, sorted by saving.
