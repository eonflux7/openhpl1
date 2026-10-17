# Roadmap

Overture Remade keeps Penumbra: Overture playing as the original did, then
improves it. Each milestone ends with a playable build.

## ✅ Milestone 0: native 64-bit build

The game builds and runs on modern 64-bit Linux. Details in
[CHANGES.md](CHANGES.md).

## Milestone 1: plays like the original

Make sure nothing regressed before changing anything.

- [ ] Full playthrough on the new build, comparing against the Steam build
- [ ] Physics tuning against Newton 1.53 behaviour: doors, drawers, ladders,
      throwing, stacking, the boulder, the worm chase
- [ ] Per-body sleep thresholds (Newton 2 only has world-wide ones): emulate
      them or retune the world defaults
- [ ] Body sleep state: `GetEnabled()` reports sleeping bodies as enabled, and
      `SetEnabled(false)` is a hard freeze in Newton 2 (contacts do not wake
      the body). Loading a save wakes every body (visible with
      `OVERTURE_PHYSLOG`)
- [ ] Old save games load and play correctly
- [ ] Audio:
  - [ ] verify the stutter fix across levels
  - [ ] verify 3D sound and the EFX/reverb path (currently disabled on Linux,
        as in the original)
- [ ] Fix the intermittent heap corruption on exit
- [ ] `level01_07_lead_mine_shafts_A/B/C` render black in the reference
      images: from the start positions the flashlight lights nothing (fog
      and shadows ruled out). Check whether the original looks the same
- [ ] Windows build (MSVC or MinGW) from the same source tree

## Milestone 2: widescreen and quality of life

- [ ] **Widescreen 2D/HUD.** The UI was drawn on a fixed 800×600 virtual
      canvas stretched to the window:
  - [x] widen the virtual canvas to the window's aspect ratio and centre the
        800×600 layout
  - [x] make full-screen overlays (fades, damage, flash, underwater, fear,
        noise filter, image trail, bloom quad) cover the whole width
  - [x] fix hard-coded 800/600/400/300 coordinates and mouse clamps
        (inventory, notebook, numerical panel, death menu, crosshair)
  - [x] play-test at 16:9: intro story, death menu, numerical panel and
        interact mode were not covered by the screenshot checks
  - [ ] decide whether the menu and logo screens should fill the sides (they
        are 4:3 art, shown centred with black borders)
- [x] FOV option (vertical FOV, 70° by default; 3D is already Hor+)
  - [x] play-test: the hiding and enemy-close effects, the save effect and
        held items at angles other than 70° were not looked at
- [x] Mouse sensitivity independent of resolution (anchored to the speed at
      1440 pixels of height)
- [x] Gamma reimplemented as a final shader pass
  - [x] play-test: change it in the options menu on a real display; only
        checked with frame dumps of values set in `settings.cfg`
- [x] Uncapped / high-refresh frame rate with interpolated rendering (logic
      stays at its fixed rate)
  - [x] play-test on a high-refresh display: only checked headless (camera
        path logged per frame, same end state as with the limit, smoke test)
  - [ ] blend skeletal animations and particles
  - [ ] a frame rate cap for when V-Sync is off
  - [ ] the main loop spins while it waits for the next step
- [x] Display, window mode and V-Sync options for SDL3 in the menu
  - [x] play-test on a real desktop: only checked under a headless gamescope
        session, which has one display; switching display, fullscreen and
        resolution and restarting was not tried
  - [ ] apply them without a restart
- [x] **Fonts** rendered from TrueType files at screen resolution, with real
      outline and glow (see CHANGES)
  - [x] play-test: credits, inventory texts and non-English text were not
        covered by the screenshot checks
  - [x] a font between handwriting and typewriter for the text on paper, and
        a pixel font for the computer text
    - [x] play-test: only the notebook pages and the pre-menu text were
          checked (headless, 720p to 1440p); the intro text with its outline
          and the map load text were not
- [ ] High-DPI and upscaled UI assets (cursor, inventory, notebook, menu art)
- [ ] Gamepad support (stretch)

## Milestone 3: renderer foundation

Rebuild what the HPL1 renderer stands on without changing how the game looks.
Every step is checked against reference images of the renderer before it.

Done and play-tested, except for the one open item below, which does not
hold up Milestone 4.

- [x] **Reference images.** Fixed viewpoints on every map, rendered
      headless and compared with the previous run, so a step that should not
      change the picture can be shown not to (`tools/refshots.py`, see
      README)
  - [x] repeatable frames (fixed time step, seeded random numbers,
        flickering lights held on)
  - [x] 2D coverage: HUD, inventory, notebook, keypad, a message and the
        main menu pages, shot over the first map at the end of a run
- [x] No GL errors or driver warnings in a debug context
      (`OVERTURE_GL_DEBUG`), so new ones stand out
  - [x] menus, inventory, notebook and other 2D screens run under it (the
        2D reference screens; death menu, intro and credits not yet)
  - [x] checked on Mesa: the test map's reference views on llvmpipe
        (software); not on Mesa's hardware drivers
- [x] Drop the fallback paths for 2003-era GPUs (the `Fallback01`/`Fallback02`
      materials and shaders, capability checks for what GL 3.3 guarantees)
- [ ] **Off-screen frame.** Render the scene into a framebuffer object:
  - [x] colour and depth/stencil textures; later passes can read the depth
  - [x] gamma reads the frame texture; the blur renders into its own
        textures (`SetRenderTarget`). Effects that read the frame while
        drawing over it (bloom, depth of field, motion blur, image trail,
        refraction) draw it into a texture of their own (Milestone 6)
  - [x] multisampling through a multisampled target and a resolve
  - [x] one fixed frame order: scene → post effects → output transform → HUD →
        gamma → window (the output transform is a plain copy until there is
        a float buffer)
  - [x] render size independent of the window size: Render Scale option
        (50–200%)
  - [ ] resolution and fullscreen changes without a restart (see
        Milestone 2). A convenience, not something later work stands on;
        left for whenever
- [x] **Float colour buffer.** A 16-bit float scene target, so light passes
      add up without being rounded to 8 bits or clipped after each one; the
      output is dithered. Same look, less banding in dark areas
  - [x] play-test: dark areas are up to a few levels brighter (light that
        8-bit rounding used to drop). Nothing was noticed in two
        play-tests; tonemapping and the lighting rewrite retune this anyway
  - the screen copies for post effects and refraction are still 8-bit, so
    bloom, depth of field and refraction clip at 1: moved to Milestone 4,
    where those effects are rewritten
- [x] **Shader system:**
  - [x] shared include files and `#define` variants instead of one file per
        combination
  - [x] per-frame values (view, projection, screen size, depth range, time)
        in one uniform block; matrices kept on the CPU instead of read back
        from GL for every program
  - [x] reload shaders while the game runs (`OVERTURE_SHADER_RELOAD`)
- [x] **GPU profiling:**
  - [x] Dear ImGui as an in-game debug overlay (F3), starting with the
        profiler's numbers
  - [x] timer queries per pass, draw-call and triangle counts, debug groups
  - [x] object labels
  - [x] fetch occlusion queries without waiting
- [x] **No fixed-function.** A core profile context (the newest the
      driver has, 3.3 at least), which RenderDoc needs:
  - [x] everything is drawn by shaders: HUD, fonts, particles, billboards,
        transparent materials and debug drawing through shaders generated
        for the texture environment the engine sets; alpha test and clip
        planes in shaders
  - [x] vertex attributes and vertex array objects; immediate mode, client
        arrays and quads replaced by streamed buffers and triangles
  - [x] play-test: looks right and runs smoothly at 2560x1440 (also checked
        against the reference images, and with anti-aliasing, render scale,
        gamma and motion blur)
  - the fixed-function shaders stand in for the old pipeline call by call;
    replaced by one default shader in Milestone 6

## Milestone 4: lighting, shadows and effects

Bring the look up to date while keeping the original art direction. New
looks are options; the original stays available.

Every item here changes the picture, so it is judged by playing, not by the
reference images. Those still guard the original look: with the new options
off, a capture should match the one before.

In this order. Lighting comes first because tonemapping, the bloom threshold
and specular all have to be tuned again if the lighting under them changes.

- [ ] **1. Gamma-correct lighting**, retuned against the original look, as
      an option:
  - [x] colour-space audit, reading only: which textures hold colour and
        which hold data (normal maps, falloff ramps, spot masks), what
        each blend mode (additive, modulate, modulate ×2, destination
        alpha) does in linear space, where colour constants come from
        (lights, ambient, fog, HUD). Known from Milestone 3: lighting is
        additive multi-pass in gamma space with the light colour clamped
        to 0..1 in the vertex shaders, and modulate and modulate ×2 do
        not survive a naive switch to linear. Written up in
        `docs/colour-space-audit.md`
  - [x] linear lighting switch (Linear Lighting in Advanced Graphics, off
        by default), converted back in the output pass. Colours are
        converted in the shaders instead of through sRGB texture formats:
        the option switches while playing and the HUD is untouched
  - [x] float screen copies and blur textures when it is on (from step 3)
  - [x] tuning values as sliders in the F2 panel: light power (the
        falloff exponent), exposure, glow (brighter-than-white halos and
        lit parts)
  - [x] play-test and retune with them: light power 1.8, exposure 1.3 and
        glow 1.45 are the defaults. Shadows may move them
  - [x] particles, alpha materials and fog cover as much as in the
        original (blend match)
  - [x] play-test the blend match and the halos again
  - [ ] sRGB texture formats, for filtering on linear values. Small gain;
        only if it shows
  - energy-conserving specular: moved to Milestone 5
- [ ] **2. Tonemapping**, colour grading. Linear lighting needs it to look
      right
  - [x] a curve in the output pass that leaves the picture alone up to a
        shoulder and rolls brighter values off towards a white point
        (Shoulder and White in the F2 panel; off until tuned)
  - left off by default after the first play-test (shoulder 1): to look
    at again with the new bloom, which is what makes values brighter than
    white
  - [ ] colour grading
- [ ] **3. Bloom** with threshold and a mip chain (next to "blur squared
      and add", which stays as Original). The original blur is measured in
      pixels horizontally and in screen widths vertically, so bloom looks
      different at each resolution and render scale
  - [x] Soft bloom as a third setting of the Bloom option: threshold,
        chain of halvings, the same at every resolution; sliders in the
        F2 panel
  - [x] play-test and settle threshold, intensity and spread (0.3, 1.55,
        0.55)
  - [ ] the outside map with these values: nearly all of it blooms
  - the copies bloom, depth of field and refraction read are float with
    linear lighting on; with it off they stay 8-bit and clip at 1, as
    originally
- [ ] **4. Soft shadows:** shadow maps instead of the hard-edged stencil
      shadow volumes, which stay as the original setting
  - [x] shadow maps for spotlights and point lights (cube maps), drawn for
        every light in every frame; Soft settings of the Shadows option
  - [x] edges: hard, a fixed blur, or a blur that widens with the distance
        to what casts the shadow
  - [x] a Shadows window in the F2 panel: every setting, the shadows
        alone, the lights in view, a test light
  - [x] play-test: looks right; a point light's map is 1024 a side by
        default. The flashlight casts shadows
  - [ ] keep the maps of lights where nothing moved, if drawing them every
        frame costs too much
- **5. The rest**, in any order:
  - [ ] Anti-aliasing and texture filtering
    - [x] SMAA 1x as an option (Edge Smoothing in Advanced Graphics, off by
          default): the reference implementation's searches, laid out
          after iMMERSE's SMAA (settings as uniforms, edges by colour or
          depth, the depth lowering the threshold); settings in the F2
          panel
    - [x] multisampling: resolved once, before the effects that are drawn
          over the finished scene; changed without a restart, up to 8x;
          alpha-tested textures get the samples too (alpha to coverage)
    - [x] texture filter: one option (trilinear, anisotropic 2x to 16x)
          for every texture, through sampler objects; anisotropic 16x by
          default
    - [x] mipmaps: alpha-tested textures keep what they cut out, cube maps
          have them, Texture Quality averages instead of dropping pixels
    - [x] Render Scale: an average of the covered pixels above 100%, a
          sharper curve below
    - [x] play-test: looks good with anisotropic 16x and 4x multisampling,
          and with SMAA
    - [ ] SMAA's pixels with edges gathered by a compute shader, as
          iMMERSE does (OpenGL 4.3), if the stencil buffer is not enough
    - [ ] multisampled scenes with linear lighting are averaged before the
          tonemapping: edges against something brighter than white stay
          stepped
  - [ ] Ambient occlusion from the depth buffer
    - [x] ground truth ambient occlusion as an option (Ambient Occlusion in
          Advanced Graphics, off by default), laid out after MXAO: depth in
          tiles, a 4x4 pattern and filter, half rate; settings in the F2
          panel
    - [x] play-test: radius, strength and "less in light"
    - [x] half rate ghosts when moving fast and in some scenes: off by
          default
    - [x] made faster after a code review and measuring each pass (see
          CHANGES): about 40% less GPU time at High with the picture as it
          was
    - [ ] if it should cost less still: fewer samples (3x4 or 3x3 now that
          no step is wasted), one filter look-up per 2x2 pixels (texture
          gather, GL 4), half rate with the old pixels moved to where they
          are now (reprojection), which would not ghost when the camera
          turns
    - [ ] the scene's depth is copied for reading up to three times a frame
          with multisampling (here, before the see-through pass, at the
          end); once would do while nothing writes depth in between
  - [ ] Depth of field and camera motion blur from depth. The current
        effects re-render geometry per object.
    - [x] Bokeh as a third setting of Depth of Field, Camera as a third
          setting of Motion Blur; settings in the F2 panel's Lens and film
          window
    - [x] play-test: blur radius and rings; shutter, longest blur and
          whether what is held in the hand stays sharp (the 0.6 m limit)
    - [ ] things that move by themselves are not blurred by Camera: a
          velocity buffer, if that is missed
  - [x] Film grain shader replacing the tiled noise-filter sprites
    - [x] Grain and Film Grain (grains of crystals that develop or not) as
          further settings of Noise Filter, under the HUD
    - [x] play-test: strength, size and colour of both
  - [ ] Refraction: works (acid water in four level 1 maps) but is off by
        default and has no menu option (`Refractions` in `settings.cfg`)
  - shaders of their own for the materials that set a texture
    environment (transparent, particles, HUD), in place of the generated
    fixed-function ones: moved to Milestone 5

## Milestone 5: materials and light sources

Milestone 4 changed how light adds up and what comes after it. This one
rewrites what is left of the 2007 shading itself: how a surface answers a
light, what it looks like where no light reaches, and what the lights the
player carries and walks past look like. Nothing in the game data is
changed, and the original stays as an option.

Judged by playing, like Milestone 4. With the new options off, a capture
should match the one before.

What the original does, for reference: one specular highlight of a fixed
width (a power of 16) for all 248 specular materials, not coloured by the
light, with the half vector made per vertex; ambient as texture × a
constant, so normal maps show only where a light reaches; falloff from a
painted ramp per light; the flashlight as a 256 px grey picture, pure
white, with no light bouncing back. There are no gloss or roughness maps:
143 materials have a specular colour map, 104 keep a specular strength in
the normal map's alpha, 160 have no normal map at all.

- [ ] **1. Surface shading** (Shading: Original / Improved, settings in the
      F2 panel's Shading window):
  - [x] specular per pixel: microfacet highlight (GGX) with Fresnel,
        coloured by the light; how shiny a surface is and how wide its
        highlight is are both read from the specular map (bright = glossy)
  - [x] the highlight widened where the normal map changes faster than the
        pixels can show, instead of sparkling
  - [x] ambient that uses the normal map: more from above and towards the
        viewer, and a sheen at grazing angles on what has specular
  - [x] play-test: looks right with linear lighting and ambient occlusion
        on; the defaults stay (reflectance 0.1, roughness 0.6 to 0.45,
        ambient 0.3 from above, 0.4 towards the viewer, sheen 0.25)
  - [x] a menu option: Options, Graphics, Lighting (a page of its own,
        the Advanced Graphics page is full)
  - [ ] gloss per material as an attribute in the material file, for what
        the specular map guesses wrong
- [ ] **2. Flashlight** (Flashlight: Original / Improved, in the same
      window):
  - [x] falloff by distance (brighter near, fading out at its reach)
        instead of the ramp that is flat for the first two thirds
  - [x] the beam's picture made by the shader: a hotspot in a dimmer
        field with a soft edge, a warm tint; no 256 px rings
  - [x] bounce: a weak light where the beam lands, as strong as the beam
        is there, without shadows
  - [x] brighter than 1: a light has a look of its own (`cLightLook`:
        brightness, tint, falloff, beam) that the light shader reads; the
        light colour itself still stops at 1
  - [x] Reach: how far the beam goes (6 to 36 m, 12 in the original), for
        seeing a little further than the original allows
  - [x] play-test: looks good, with the glow stick, the flare and the
        lamps of step 3
  - [x] a menu option, on the Lighting page
- [ ] **3. Glow stick, flare and the lamps of the world** (Lamps: Original
      / Improved, in the same window). The kind of lamp is read from the
      name of its entity file; nothing in the data is changed:
  - [x] a smooth flicker of brightness and a slight sway of position for
        flames (paraffin lamps, the flare in the hand and thrown), so their
        shadows move
  - [x] a colour per kind of lamp: flames orange, bulbs and emergency
        lights warm, fluorescent tubes and tunnel lights cold, the glow
        stick green; one slider from the original's colours to these
  - [x] brighter near the lamp: the flashlight's falloff by distance in
        place of the ramp, which lights most of a lamp's reach evenly
  - [x] tube lights for the fluorescents: long highlights (with Shading:
        Improved)
  - [x] play-test: looks good; the defaults stay
  - [x] the halo of a flame flickering with it: a light's halos take its
        look's tint and brightness
  - [x] lights that are part of the maps and not of a lamp entity (56):
        the falloff by distance for those that reach 20 m at most and
        have no falloff picture of their own; the furnace's lights are
        flames; colours stay, since nothing says what kind they are
  - [x] a menu option, on the Lighting page
  - [x] play-test of the halos, the lights of the maps and the menu
        page: all good
- [ ] **4. Later, in any order:**
  - [x] halos drawn by a shader (options, off by default): the glow from
        the shape of the halo's picture, without its steps; hidden by
        what is in front of the lamp as read from the depth buffer
  - [x] light shafts as bodies of lit air in place of flat sheets: soft
        where they meet the scene, right from every side
  - [x] particles: soft where they meet the scene and close to the eye;
        smoke, mist, dust and snow lit by the lights around them
  - [x] play-test of the three
  - [ ] particles, later: shadows on them (the flashlight's shadow map),
        detail from the shader for the 64 pixel pictures, stretch by speed
        and swirling for smoke
  - [ ] the flashlight's beam seen in the air (the cone marched through its
        shadow map)
  - [ ] see-through materials other than particles lit by the lights;
        shaders of their own in place of the generated fixed-function ones
        (from Milestone 4)
  - [ ] water and the environment map material (3 materials)

## Milestone 6: the rest of the 2007 renderer

Milestones 3 to 5 put a modern frame and new looks on top of the HPL1
renderer; underneath, the engine still talks to it as to OpenGL 1.x. Every
draw goes through texture environments, an alpha test, a matrix stack and
quads, which shaders generated for each combination stand in for
(`FixedFunctionShaders`). Materials describe themselves as numbered passes
of texture units, ask for programs by their Cg file names, and set uniforms
by name. This milestone replaces that with code written for what the
renderer is now. It can run alongside Milestone 5.

Nothing here changes the picture to the eye: every step is checked
against the reference images, with the new options off and with them on
(after step 4, the improved look only). It need not be exact; a few levels
here and there are fine, and are noted. Motion blur, render scale and the
effects lab are checked by the steps that touch them.

In this order: each step removes callers of what a later one deletes.

- [x] **1. Dead code.** Remove what the game never reaches:
  - [x] the 2D engine of HPL1: 2D worlds, tile maps, 2D lights and their
        stencil shadows, 2D cameras, 2D bodies and colliders, image
        entities, 2D particles, background images and `cRenderer2D`
  - [x] the `Diffuse2D` material
  - [x] low-level functions nothing calls (`DrawRect`, `DrawTri`, the texture
        coordinate batching), the commented-out stencil functions, and
        commented-out code in the renderer
- [x] **2. Post effects read the frame, not a copy of it.** Bloom, depth of
      field, motion blur, ambient occlusion, image trail and refraction
      copy the screen with `CopyContextToTexure`; they read the scene
      texture instead, or draw into a target of their own (the open item
      of Milestone 3's off-screen frame):
  - [x] rectangle textures (`sampler2DRect`, coordinates in pixels)
        replaced by 2D textures
  - [x] the original blur, depth of field and image trail drawn by
        shaders of their own instead of texture environments
- [x] **3. Shaders for every material.** Modulative, modulative ×2, the
      environment map and the 2D materials of the HUD (alpha, additive,
      smoke, font) get shaders of their own, in place of texture
      environments and the `iGLStateProgram` state objects. So do the
      remaining texture environment calls in the renderer and the game
      (the fog, the item outline in `MapHandler`). Then:
  - [x] `SetTextureEnv`, `SetTextureConstantColor`, `SetActiveTextureUnit`,
        the alpha test and `SetColor` removed from `iLowLevelGraphics`
        (the alpha test is one cut-off now, `SetAlphaCutoff`; the low
        level needs it for alpha to coverage)
  - [x] `FixedFunctionShaders` and its state uniform block removed (the
        `hpl_State` block stays: every program reads it)
  - [x] the light shader without its look-up textures: the normalization
        cube map (`normalize()` does it) and the spotlight's 1D texture
        that cuts off what is behind it; the 3D attenuation texture if
        nothing reads it. The falloff ramps are game data and stay
- [x] **4. One look.** The improved look becomes the only one, so the
      steps after this refactor one renderer instead of two. The original
      look was the reference the improved one was tuned against; the tag
      `original-look` marks the last commit with it, and its reference
      images are kept (`captures/original-look` in the Windows kit). Who
      wants the 2007 look exactly can play the 2007 game:
  - [x] the improved settings are the defaults; `refshots.py capture`
        takes the improved look without extra settings, and its images
        are the reference from here on
  - [x] linear lighting only: the encoded colour path and
        `SetLinearSource` go. `HPL_LINEAR_COLOR` stays to mark the shaders
        that write linear values (the materials still write screen
        values), and so does the alpha matching for blending: it is part
        of the improved look
  - [x] shadow maps only: stencil shadow volumes go (`ShadowType`, the
        extrusion shader, silhouettes found on the CPU)
  - [x] the bloom chain, bokeh depth of field and camera motion blur only:
        the original bloom, depth of field, motion blur and their 8-bit
        screen copies go
  - [x] the improved shading, flashlight, lamps, halos, shafts, particles
        and player effects only: their `...Improved` switches and the
        original paths behind them go (with the halos' occlusion queries
        and the image trail)
  - [x] what is about speed or taste stays an option: shadows (off and
        quality), ambient occlusion, SMAA, FSAA, motion blur, bloom, depth
        of field, grain, render scale, gamma
- [ ] **5. No matrix stack.** Model, view and projection given to each
      draw instead of pushed, popped and multiplied in place; `eMatrix`
      removed. The renderer, post effects, lights, the GUI and the game
      (enemies, the player, the HUD weapon, the intro) use it today
- [ ] **6. Materials and programs:**
  - [ ] one program object of vertex and fragment shader together, in
        place of the pair the Cg-era interface binds one by one
  - [ ] shaders asked for by their GLSL name and `#define`s, not a Cg file
        name translated (`CgNameToShaderBase`)
  - [ ] uniform locations looked up once, values for a material in a
        uniform block, instead of about 200 calls setting them by name
  - [ ] the draw order by one sort key per draw in place of the twelve
        level state tree of `cRenderState`
- [ ] **7. One sprite batcher for 2D.** The HUD, GUI, fonts and fades drawn
      by a batcher of textured triangles with a blend mode per batch, in
      place of `AddVertexToBatch`, the quad batches and `DrawQuad`
- [ ] **8. Vertex buffers and textures:**
  - [ ] vertices interleaved in one buffer, indices in an element buffer,
        triangles only (quads are turned into triangles when loaded)
  - [ ] named attributes instead of the `hpl_MultiTexCoord0-4` slots
  - [ ] textures with immutable storage; animated textures as one texture
        array instead of one texture per frame
- [ ] **9. `cLowLevelGraphicsSDL` split up.** 3300 lines doing the window,
      GL state, scene and output framebuffers, the final passes (gamma,
      SMAA, output, player effects), shadow map targets and debug drawing:
      a class for each
- [ ] **10. Billboards, beams and particles** written the way the rest
      now is, instead of geometry built on the CPU every frame

## Later / ideas

- Penumbra: Black Plague and Requiem on the same engine: under way, see
  [PORTING-ROADMAP.md](PORTING-ROADMAP.md)
- Newer Newton Dynamics or a different physics engine, if Milestone 1 tuning
  is not enough
- Packaging (Flatpak) and an installer that finds the game data automatically
