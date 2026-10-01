# Colour-space audit

The first step of Milestone 4 (gamma-correct lighting). Nothing was changed:
this is what the renderer and the game data do today, read from the code and
from the Penumbra: Overture data, and what follows from it for the linear
lighting option. "Encoded" below means the 8-bit values as stored in the
image files and shown on screen (sRGB, roughly a 2.2 power curve); "linear"
means proportional to light.

## Summary

- Everything is computed on encoded values today. No texture is decoded, the
  scene buffer holds encoded values, the output pass only dithers.
- A chain of multiplications gives the same picture in either space, as long
  as every factor is converted: (a·b)^2.2 = a^2.2 · b^2.2. Texture × ambient,
  texture × light colour × falloff, sky × sky colour, and the modulate blend
  are all such chains. So one light on one surface can be made to look
  exactly as it does now.
- What changes is every place where values are **added or mixed**: lights
  overlapping, ambient plus light, specular on diffuse, illumination maps,
  additive halos and particles, alpha blending, fog, and all filtering
  (mipmaps, anti-aliasing, blur, render scale). That is the actual content of
  the linear switch, and what has to be retuned.
- Modulate and modulate ×2 survive after all, if the constants in them are
  converted (the 2 and the neutral grey 0.5).
- Two things have to come with the switch that the roadmap had later: the
  8-bit screen copies (linear values in 8 bits band badly in the dark), and a
  way for the generated shaders to know whether they draw into the scene or
  into the 8-bit output.

## The frame today

In `cRenderer3D::RenderWorld`, into the 16-bit float scene buffer:

| Pass | Blend | What is written |
|---|---|---|
| Z / ambient | replace | diffuse texture × ambient colour × sector ambient; alpha = texture alpha |
| Lights, one pass per light | add | diffuse × light colour × N·L × falloff ramp (× spot texture), plus specular |
| Diffuse | add | illumination map × vertex colour |
| Fog | alpha | fog colour, alpha from a linear ramp over distance |
| Sky box | replace | cube map × sky colour |
| Transparent, back to front | per material | see blend modes below |

Then the game's `OnPostSceneDraw` (item flash, intro pictures, menu
backgrounds: full-screen textures through the generated shaders), the post
effects (depth of field, motion blur, bloom, image trail), the output pass
(dither to 8 bits), the HUD and menus in the 8-bit output buffer, and the
gamma option (`pow(colour, 1/gamma)`) on the finished 8-bit picture.

Every fragment shader's result is clamped to 0..1 (`kFragmentEpilogue` in
`GLSLProgram.cpp`), as the fixed-function pipeline did. That also removes the
negative values the light shader produces: N·L is not clamped in
`hpl1_light.fragment`.

The scene alpha channel is used as a mask (alpha test at 0.6, the water
refraction mask), never as colour. It is not affected.

## Textures

All textures are loaded as plain `GL_RGB8` / `GL_RGBA8` / `GL_R8`
(`SDLTexture.cpp`). All 1077 texture units in the 593 material files have
`Compress="false"`; the images are jpg (946), tga (274), bmp (186), png (22).

| Texture | Count | Holds | Decode? |
|---|---|---|---|
| Diffuse (no suffix) | all materials | colour; alpha = cut-out or blend amount | yes |
| `_spec`, specular colour map | 143 materials | colour | yes |
| `_illum`, illumination map | 38 materials | colour (emitted light) | yes |
| Spotlight picture (`ProjectionImage`) | 15 light files | colour; alpha scales specular | yes |
| Sky box and reflection cube maps | few | colour | yes |
| `_bump`, normal map | 298 materials | vectors; alpha = specular strength (BumpSpecular, 104 materials) | no |
| `_ref`, refraction map | water, acid | offset vectors | no |
| Water "specular" unit | 2 materials | a mask for the refraction shader | no |
| Falloff ramp (1D) | 4 files | brightness over distance, painted by eye | no; see below |
| Normalisation cube map, spot negative-reject, fog ramps | engine | vectors, step, alpha | no |
| Screen copies, blur textures | engine | whatever the scene buffer holds | see below |
| HUD, fonts, menus, GUI atlas | 2D | colour, drawn into the 8-bit output | stays encoded |

Notes:

- The falloff ramps are not straight lines. `core_falloff_linear.jpg` reads
  255, 244, 216, 179, 138, 96, 57, 24, 1 at eighths of the radius; the
  spotlight ramps stay at 255 for the first half and then drop. They were
  painted to look right on encoded values. Alpha channels (specular strength
  in normal maps and spot pictures) are in the same position: OpenGL never
  decodes alpha. For these the shader has to choose an exponent: 2.2 keeps
  today's look, 1 treats them as physical amounts. That exponent is a tuning
  knob.
- The texture cache is keyed by file name, and the game loads pictures with
  `Create2D` both for the scene and for the HUD. A per-texture "this is
  sRGB" format would leak from one use to the other. `GL_TEXTURE_SRGB_DECODE_EXT`
  avoids this: load colour textures in sRGB formats and switch decoding on or
  off per use (it is a texture/sampler parameter). With decoding skipped the
  texture behaves exactly as today, which is also the "option off" path.
- OpenGL's sRGB decode is the piecewise sRGB curve, not a 2.2 power. The
  "same picture" rule above is exact only for a pure power, so with sRGB
  formats products differ slightly in the darkest values. Small, and tuned
  over anyway; the alternative (decoding with `pow` in the shader) keeps
  filtering wrong.
- `cTextureManager::CreateAttenuation` (a 3D falloff texture) has no caller.

## Colour constants

| Constant | Comes from | Values in the data |
|---|---|---|
| Light colour | Collada `<color>` of the light; `intensity` is ignored | 97 lights in maps and models; none above 1, 57 have a component at exactly 1.0, 22 are grey |
| Light specular amount (colour alpha) | point lights: node scale Y; spots: `Specular` in the `.lnt` file | 0.6–1.0 in the light files |
| Light fades and flicker | scripts (`FadeLight3D`), lamps, the flashlight and glowstick code | up to 1.0 |
| Ambient | `SetAmbientColor` in the level scripts, × sector ambient | 0.06–0.28, mostly 0.1 |
| Fog colour | `SetFogProperties` in the level scripts | fog is turned on in 12 places |
| Sky colour | `SetSkyboxColor` | 0.5–1.0 |
| Vertex colours | billboards (halo fade), particles | particle files have multipliers up to 5; clamped to 1 in the shaders |
| Flash, fades, HUD colours | game code | 2D, encoded |

All of these are multipliers chosen by eye on encoded values. An ambient of
0.1 means "a tenth of the texture's screen value", which is 0.1^2.2 ≈ 0.006
of its light. Used unconverted in linear, ambient 0.1 would come out as 35%
grey on screen instead of 10%. So every constant is converted where it enters
a shader, not in the data.

The light colour is clamped to 0..1 in `hpl1_light.vertex`. No light in the
data needs more than 1, so the clamp only matters for fades that overshoot.
It can go once lights brighter than 1 are wanted.

## Blend modes in linear

| Mode | GL blend | Used by | In linear |
|---|---|---|---|
| Replace | src | ambient pass, sky, Flat | same picture, if texture and constant are both converted |
| Add | src + dst | light passes, illumination, Additive (52 materials + 14 billboards), particles | changes: this is the point |
| Alpha | src·a + dst·(1−a) | Alpha (39 materials), solid fog | changes: edges of light-on-dark get heavier, dark-on-light thinner |
| Mul | dst · src | Modulative (18), water (2) | same picture if the texture is converted. The fogged variant fades towards white, which is 1 in both spaces |
| MulX2 | 2 · dst · src | ModulativeX2 (15) | same picture if the 2 becomes 2^2.2 ≈ 4.6 (the float buffer allows it) and the fogged variant's neutral 0.5 becomes 0.5^2.2 ≈ 0.22 |
| DestAlphaAdd | src·dst.a + dst | no material | – |

What "changes" means in numbers, with every factor converted so a single
term looks as before:

- Two lights of 0.6 on screen overlapping: 1.2 today (clipped to white);
  0.82 in linear. Overlaps get darker and stop blowing out.
- Ambient 0.1 plus a light of 0.5: 0.60 today; 0.51 in linear. Ambient still
  fills the shadows but barely lifts lit surfaces.
- If instead N·L and the falloff ramp are used as physical amounts (exponent
  1), half-lit comes out as 0.73 rather than 0.5: lights look wider and
  flatter, with a harder edge at the radius.

So the linear picture is darker where things add up and, depending on the
falloff exponent, brighter in the mid-tones of each light. Exposure and
tonemapping (step 2) are where the overall level is set again.

## Specular

`pow(N·H, 16)` × light alpha × (spot alpha) × specular colour map, or ×
normal-map alpha, or × 1 (DiffuseSpecular, one material). Added to the
diffuse term in the same pass and clamped. The exponent is fixed for all
materials and there is no normalisation, so "energy-conserving specular" has
only the specular map and the light's specular amount to work with; a
per-material gloss value would be new data.

## Post effects and screen copies

- The screen copies that depth of field, motion blur, image trail and
  refraction read are 8-bit, and so are the 256×256 blur textures. With
  linear values in them, the dark end of the picture has only a handful of
  steps: visible banding, in a game that is mostly dark. They need to be
  float (already on the roadmap under bloom) **before or with** the linear
  switch, not after.
- Bloom squares the blurred picture and adds it. On linear values it would
  pick out a different set of pixels. It is replaced in step 3; until then
  it can convert to encoded values, do its sum, and convert back, to stay as
  it is.
- Depth of field, motion blur, image trail and the blur are mixes. They
  change slightly (bright points spread more). No action needed.
- The output pass becomes the place where linear turns into encoded values
  (and later tonemapping), with the dither after it. The gamma option stays
  last, on the 8-bit picture.

## What the next steps need

1. Colour textures loaded in sRGB formats, with decoding switched per use
   (`GL_TEXTURE_SRGB_DECODE_EXT`). Off everywhere = today's picture, which
   the reference images can confirm.
2. One "the bound target is linear" state, on between `BeginScene` and
   `EndScene` when the option is on, reaching every shader through the
   `hpl_State` block. The generated texture-environment shaders serve both
   the scene (transparent materials, particles, billboards, the game's
   full-screen pictures) and the 8-bit HUD, so they cannot decide by
   themselves. With it they convert constants and vertex colours.
3. Float screen copies and blur textures.
4. The per-shader clamp to 0..1 lifted in linear mode (keep the clamp at 0),
   N·L clamped in the light shader.
5. A falloff/N·L exponent between 1 and 2.2 as the first tuning value, then
   exposure in the output pass.

With 1–4 and the exponent at 2.2, a room lit by one light should match the
original closely, and the difference to judge by playing is exactly the
adding and mixing listed above.

## What was built

Points 2–5 as listed (see "Linear lighting" in `CHANGES.md`). Point 1 was
done differently: the colours are converted in the shaders with a 2.2
power instead of through sRGB texture formats. Since a shader's result
converted as a whole equals the product of its converted factors, most
shaders did not have to change at all: the conversion sits in the code
appended to every fragment shader. That removes the cache problem and the
sRGB-curve mismatch described under Textures, and lets the option switch
without reloading anything. Filtering on linear values, the one thing sRGB
formats would add, is left for later.

The reference views confirm the numbers above: single lights look the same,
overlaps and ambient-plus-light are darker, and additive halos lose most of
their glow because they no longer push the picture past white.
