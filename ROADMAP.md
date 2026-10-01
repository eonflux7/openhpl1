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
- [ ] Old save games load and play correctly
- [ ] Audio:
  - [ ] verify the stutter fix across levels
  - [ ] verify 3D sound and the EFX/reverb path (currently disabled on Linux,
        as in the original)
- [ ] Fix the intermittent heap corruption on exit
- [ ] Clean up remaining GL debug warnings
- [ ] Windows build (MSVC or MinGW) from the same source tree

## Milestone 2: widescreen and quality of life

- [ ] **Widescreen 2D/HUD.** The UI is drawn on a fixed 800×600 virtual
      canvas stretched to the window:
  - [ ] widen the virtual canvas to the window's aspect ratio and centre the
        800×600 layout
  - [ ] make full-screen overlays (fades, damage, flash, underwater, fear,
        noise filter, image trail, bloom quad) cover the whole width
  - [ ] fix hard-coded 800/600/400/300 coordinates and mouse clamps
        (inventory, notebook, numerical panel, death menu, crosshair)
- [ ] FOV option (vertical FOV is fixed at 70°, and 3D is already Hor+)
- [ ] Mouse sensitivity independent of resolution
- [ ] Gamma reimplemented as a final shader pass
- [ ] Uncapped / high-refresh frame rate with interpolated rendering (logic
      stays at its fixed rate)
- [ ] Display, window mode and V-Sync options for SDL3 in the menu
- [ ] High-DPI and upscaled UI assets (fonts are already upscaled)
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
