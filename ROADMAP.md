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
- [ ] Clean up remaining GL debug warnings
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
- [ ] Mouse sensitivity independent of resolution
- [x] Gamma reimplemented as a final shader pass
  - [ ] play-test: change it in the options menu on a real display; only
        checked with frame dumps of values set in `settings.cfg`
- [ ] Uncapped / high-refresh frame rate with interpolated rendering (logic
      stays at its fixed rate)
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

## Milestone 3: renderer

Bring the HPL1 renderer up to date while keeping the original art direction.

- [ ] Render the scene into an off-screen buffer (FBO), replacing the copies
      from the back buffer
- [ ] HDR render target, tonemapping, colour grading
- [ ] Bloom with threshold and a mip chain (replaces "blur squared and add")
- [ ] SMAA (helps alpha-tested geometry, which MSAA does not)
- [ ] Ambient occlusion (SSAO/GTAO) from the depth buffer
- [ ] Depth of field and camera motion blur from depth. The current effects
      re-render geometry per object.
- [ ] Film grain shader replacing the tiled noise-filter sprites
- [ ] Rewrite the lighting shaders:
  - [ ] energy-conserving specular
  - [ ] gamma-correct lighting
  - [ ] drop the fallback paths for 2003-era GPUs
- [ ] **Soft shadows:** shadow maps (PCF/PCSS) instead of the hard-edged
      stencil shadow volumes, or penumbra-blurred stencil shadows as an
      interim step
- [ ] Retire the remaining fixed-function code; GL 3.3+ core profile

## Later / ideas

- Penumbra: Black Plague and Requiem on the same engine (they share most of
  HPL1)
- Newer Newton Dynamics or a different physics engine, if Milestone 1 tuning
  is not enough
- Packaging (Flatpak) and an installer that finds the game data automatically
