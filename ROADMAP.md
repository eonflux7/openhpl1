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
  - [ ] play-test at 16:9: intro story, death menu, numerical panel and
        interact mode were not covered by the screenshot checks
  - [ ] decide whether the menu and logo screens should fill the sides (they
        are 4:3 art, shown centred with black borders)
- [x] FOV option (vertical FOV, 70° by default; 3D is already Hor+)
  - [ ] play-test: the hiding and enemy-close effects, the save effect and
        held items at angles other than 70° were not looked at
- [x] Mouse sensitivity independent of resolution (anchored to the speed at
      1440 pixels of height)
- [x] Gamma reimplemented as a final shader pass
  - [ ] play-test: change it in the options menu on a real display; only
        checked with frame dumps of values set in `settings.cfg`
- [x] Uncapped / high-refresh frame rate with interpolated rendering (logic
      stays at its fixed rate)
  - [ ] play-test on a high-refresh display: only checked headless (camera
        path logged per frame, same end state as with the limit, smoke test)
  - [ ] blend skeletal animations and particles
  - [ ] a frame rate cap for when V-Sync is off
  - [ ] the main loop spins while it waits for the next step
- [x] Display, window mode and V-Sync options for SDL3 in the menu
  - [ ] play-test on a real desktop: only checked under a headless gamescope
        session, which has one display; switching display, fullscreen and
        resolution and restarting was not tried
  - [ ] apply them without a restart
- [x] **Fonts** rendered from TrueType files at screen resolution, with real
      outline and glow (see CHANGES)
  - [ ] play-test: notebook notes and task list (`cour.fnt`), credits,
        inventory texts and non-English text were not covered by the
        screenshot checks
- [ ] High-DPI and upscaled UI assets (cursor, inventory, notebook, menu art)
- [ ] Gamepad support (stretch)

## Milestone 3: renderer foundation

Rebuild what the HPL1 renderer stands on without changing how the game looks.
Every step is checked against reference images of the renderer before it.

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
        textures (`SetRenderTarget`). The copies left are of effects that
        read the frame while drawing over it (bloom, depth of field, motion
        blur, image trail, refraction)
  - [x] multisampling through a multisampled target and a resolve
  - [x] one fixed frame order: scene → post effects → output transform → HUD →
        gamma → window (the output transform is a plain copy until there is
        a float buffer)
  - [x] render size independent of the window size: Render Scale option
        (50–200%)
  - [ ] resolution and fullscreen changes without a restart (see
        Milestone 2)
- [x] **Float colour buffer.** A 16-bit float scene target, so light passes
      add up without being rounded to 8 bits or clipped after each one; the
      output is dithered. Same look, less banding in dark areas
  - [ ] play-test: dark areas are up to a few levels brighter (light that
        8-bit rounding used to drop), check that nothing looks washed out
  - [ ] screen copies for post effects and refraction are still 8-bit, so
        bloom, depth of field and refraction clip at 1
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
  - [ ] play-test: checked against the reference images and with stencil
        shadows, anti-aliasing, gamma and motion blur; not on a real display
  - [ ] the fixed-function shaders stand in for the old pipeline call by
        call; materials that set a texture environment could have shaders of
        their own instead (Milestone 4, when their look changes)

## Milestone 4: lighting, shadows and effects

Bring the look up to date while keeping the original art direction. New
looks are options; the original stays available.

- [ ] Tonemapping, colour grading
- [ ] Rewrite the lighting shaders:
  - [ ] gamma-correct lighting, retuned against the original look, as an
        option:
    - [ ] colour space: which textures hold colour and which hold data
          (normal maps, falloff ramps, spot masks), what each blend mode
          (additive, modulate, modulate ×2, destination alpha) does in
          linear space, where colour constants come from (lights,
          ambient, fog, HUD)
    - [ ] colour-space flag on textures, sRGB formats for the colour ones
    - [ ] linear lighting switch, converted back in the output pass
  - [ ] energy-conserving specular
- [ ] **Soft shadows:** shadow maps (PCF/PCSS) instead of the hard-edged
      stencil shadow volumes, or penumbra-blurred stencil shadows as an
      interim step
- [ ] Bloom with threshold and a mip chain (replaces "blur squared and add").
      The current blur is measured in pixels horizontally and in screen
      widths vertically, so bloom looks different at each resolution and
      render scale
- [ ] SMAA (helps alpha-tested geometry, which MSAA does not)
- [ ] Ambient occlusion (SSAO/GTAO) from the depth buffer
- [ ] Depth of field and camera motion blur from depth. The current effects
      re-render geometry per object.
- [ ] Film grain shader replacing the tiled noise-filter sprites
- [ ] Refraction: works (acid water in four level 1 maps) but is off by
      default and has no menu option (`Refractions` in `settings.cfg`)

## Later / ideas

- Penumbra: Black Plague and Requiem on the same engine (they share most of
  HPL1)
- Newer Newton Dynamics or a different physics engine, if Milestone 1 tuning
  is not enough
- Packaging (Flatpak) and an installer that finds the game data automatically
