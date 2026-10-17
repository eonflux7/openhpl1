# AO implementation review

> Archived on 2026-10-18. This is a historical snapshot, not current
> status or agent instructions. See the [current roadmap](../../ROADMAP.md)
> and [archive index](README.md) for context. Paths under old build
> directories and `/tmp` may no longer exist.

Reviewed on 2026-10-02 for an RTX 2060 at 2560×1440 using the High preset.
The reported AO cost is approximately 40% of the GPU frame. This is a code
review; no GPU benchmarks were run and no implementation files were changed.

## Assessment

This is a solid GTAO implementation, with sensible depth deinterleaving,
compact `R16F`/`RG16F` buffers, and coherent horizon integration. There are
several concrete optimization opportunities. Optimize the existing
implementation before replacing it.

At 2560×1440, High uses up to **133 million search taps per frame**:
three slices × six steps × two sides. Early exits and skipped samples
reduce that, but the cost is understandable.

## Optimization priorities

### 1. Simplify the horizon integral without changing its mathematics

The slice integration in
[hpl1_PostEffect_AO.fragment](../../shaders/hpl1_PostEffect_AO.fragment), around
lines 198–242, evaluates one `atan` and two additional `cos` calls per
slice. The double-angle identities eliminate those. Each side’s
contribution, including the current `normalLength * 0.25` weighting,
becomes:

```glsl
float c = clamp(horizon, -1.0, 1.0);
float s = sqrt(max(0.0, 1.0 - c * c));
float contribution = 0.5 * (
    (1.0 - c * c) * normalEye
    + way * (acos(c) - c * s) * normalSide);
```

Accumulate these contributions directly into `open`, replacing both the
current per-side `sum` expression and the subsequent
`normalLength * 0.25 * sum` accumulation. Keep the final division by
`sliceNum`, clamping, strength, and fade. `normalAngle` and `cosNormal`
are then unnecessary. `sinNormal` is still needed to initialize the
horizons.

Numerical comparisons matched the current expression: 400,000 valid
horizon comparisons in double precision had a maximum absolute error
of approximately `1.12e-13`; a separate 40,000-comparison float32
spot-check had a maximum absolute difference of approximately `3.99e-7`.
These checks establish numerical agreement, not shader performance or
visual validation. This is a strong first experiment, though its GPU
benefit needs measurement.

### 2. Combine the last filter pass with compositing

The pass sequence in
[RendererPostEffects.cpp](../../engine/sources/graphics/RendererPostEffects.cpp),
around lines 1230–1268, always writes the final filtered AO texture,
then reads it again to blend. With the default one filter pass, calculate
the filtered factor directly in the composite shader. This removes one
full-screen intermediate write and its subsequent read. With two filter
passes, retain the first and combine the second with compositing.

MXAO also performs its final filtering inside compositing.
[MXAO source](https://github.com/martymcmodding/iMMERSE/blob/main/Shaders/MartysMods_MXAO.fx)

### 3. Avoid fetching the same rounded tap repeatedly

The search loop in
[hpl1_PostEffect_AO.fragment](../../shaders/hpl1_PostEffect_AO.fragment), around
lines 215–234, skips the center pixel, but different steps can round to
the same non-center coordinate. Because their calculations are identical,
repeated taps contribute nothing. Track the previous tap within each
side’s search and skip duplicates.

This matters most at smaller projected radii. Balance the saved work
against the extra comparison and registers.

### 4. Use texture gathers for the filter

Each filter pass in
[hpl1_PostEffect_AO.fragment](../../shaders/hpl1_PostEffect_AO.fragment), around
lines 269–292, performs 16 neighborhood fetches plus the center fetch:
about **63 million fetch instructions at 1440p**. Four 2×2 gathers per
channel can replace the neighborhood reads. That reduces texture
instructions, although it still reads the same texels.

MXAO uses this approach. Preserve the current explicit edge clamping
and provide a fallback for the renderer’s OpenGL 3.3 baseline.
[MXAO filtering](https://github.com/martymcmodding/iMMERSE/blob/main/Shaders/MartysMods_MXAO.fx),
[GLSL gather specification](https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.20.pdf)

### 5. Specialize common quality presets and precompute their directions

The loop limits are uniforms, and the slice directions in
[hpl1_PostEffect_AO.fragment](../../shaders/hpl1_PostEffect_AO.fragment), around
lines 188–191, recompute sine/cosine per pixel despite being identical
throughout a tile.

Preset-specific shader variants with precomputed directions could help
the compiler schedule the search. Retain the dynamic variant for Custom,
and avoid assuming that fully unrolling everything will help.

## Larger changes

**Half-width, half-height AO with depth-aware upsampling** reduces
evaluated pixels to one quarter. This requires careful handling of thin
geometry and contact detail.

Alternatively, a compute path could search deinterleaved depth but write
AO directly into ordinary screen coordinates, eliminating the separate
reinterleave pass. These are more substantial changes.

## Quality and correctness observations

- **The default filter has a half-pixel offset.** Its window spans
  `[-2, +1]` on both axes. On constant depth, this shifts the smoothing
  by −0.5 pixels; the second mirrored pass compensates, but the default
  runs only one. A centered kernel would avoid this.
- **Half-rate history needs invalidation.** The activation/settings
  setters in
  [RendererPostEffects.h](../../engine/include/graphics/RendererPostEffects.h),
  around lines 296–298, do not reset `mbAoFilled`. Re-enabling AO or
  changing settings can therefore mix stale values into the first frame.
  Camera cuts need the same treatment.
- **The depth bias deserves tuning.** The 0.2% offset in
  [hpl1_PostEffect_AO.fragment](../../shaders/hpl1_PostEffect_AO.fragment), around
  line 174, moves the point 6 cm toward the camera along the depth axis
  at 30 m. It suppresses precision artifacts, but can weaken contact
  occlusion.
- **The lighting application is heuristic.** Multiplying the lit scene
  attenuates direct lighting and specular contributions too. `lightFade`
  mitigates that using brightness, which also depends on albedo.
  Applying AO during ambient lighting would be more faithful and could
  remove the scene copy.

## Sample-count comparison and recommended starting point

XeGTAO’s High preset uses **18 search samples per pixel**, versus this
implementation’s 36. Its sampling and denoiser differ, so halving the
sample count here will not necessarily preserve quality, but trying
**3 slices × 3 steps** is worthwhile.
[XeGTAO sampling discussion](https://github.com/GameTechDev/XeGTAO#resolution-and-sampling)

Start with the **integral simplification and filter/composite fusion**,
followed by duplicate-tap removal. Validate image quality and GPU time
for each change before proceeding to larger architectural changes.
