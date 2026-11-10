# Changes from the original

What OpenHPL1 changes compared to the code Frictional Games released
(FrictionalGames/HPL1Engine @ `5e441bb` and FrictionalGames/PenumbraOverture
@ `6a6953a`, imported unmodified in the first commit) and the last official
builds. This is a summary by area. Each change's reasons and its validation are
in its commit message. The detailed log up to 2026-11-10 is archived in
[docs/archive/CHANGES-2026-11.md](docs/archive/CHANGES-2026-11.md).

## Games

- One executable, `openhpl1`, runs Overture, Black Plague and Requiem. A game
  chooser finds installations in Steam, GOG or any folder; `--game=` and
  Options → Change Game switch between them.
- Black Plague and Requiem game code is ported to match the shipped binaries
  (experimental; see [the porting notes](docs/black-plague-port.md)).
- Settings, saves and logs are kept in separate `...Remade` personal
  folders, so the original games' files are never touched.
- First run applies the High graphics preset at desktop resolution and turns
  V-Sync and environmental audio on. Existing settings files are untouched.
- Dogs have Original and Improved AI (Options → AI). Both share animation
  and stability fixes; see [Dog foundations](docs/dog-foundations.md).

## Platform and build

- Native 64-bit Linux and Windows builds with CMake, C++17 and current
  GCC, Clang and MSVC. Release packages for both are built by CI.
- SDL 1.2 → SDL3: native Wayland, raw relative mouse input, borderless
  fullscreen at desktop size, the window close button and SIGTERM quit.
- Opens on the primary display (the largest on Wayland); display,
  resolution and fullscreen are options.
- GLee/GLU → glad; FLTK dialogs → SDL message boxes; SDL_ttf dropped.
- Environment variables and CMake options use the `OPENHPL_` prefix.

## Rendering

- Nvidia Cg and fixed-function OpenGL → a GLSL core-profile renderer (GL 4.3,
  or 3.3 with the texture-storage extensions). It has one look and none of
  the 2007 renderer's switches.
- Linear lighting with tonemapping into a 16-bit float scene buffer.
- Soft shadow maps for spot and point lights in place of stencil shadow
  volumes, with unchanged shadow maps kept between frames
  ([shadow cache](docs/shadow-cache.md)).
- GGX highlights, a reworked flashlight beam and lamp lights, and
  shader-drawn halos, light shafts and particles.
- Ambient occlusion from the depth buffer, soft bloom, bokeh depth of field,
  camera motion blur and shader film grain.
- SMAA, multisampling, render scale, and anisotropic filtering up to 16x.
  Graphics presets: High, Medium and Low.
- Gamma is a final shader pass affecting only the game window.
- Fixed: fogged maps drawn as flat fog colour, sheared odd-width images, and
  the black screen and transparent window on modern drivers and Wayland.

## Screen, text and HUD

- Native widescreen: the 800×600 layout is scaled uniformly and centred, and
  full-screen overlays cover the whole window. Field of view is an option
  (Hor+, 70° vertical by default).
- Text is drawn from TrueType fonts with FreeType at its on-screen size. It
  wraps where the original did; the computer text is a pixel font.
- High-resolution HUD images, an inventory drawer with health and battery
  bars, a remastered notebook, key caps for keys in tips, and door, ladder and
  dot crosshairs. `OPENHPL_ORIGINAL_HUD=1` keeps the original HUD.
- Frame rates above 60 FPS: game logic stays at 60 steps a second and
  rendering interpolates between steps; mouse look and cursors don't lag.
- Turning speed no longer depends on the resolution or aspect ratio.
- Text from language files and notes is no longer used as a printf format
  (a '%' in a note crashed Black Plague).

## Audio

- OALWrapper → a direct OpenAL Soft backend ([audio](docs/audio.md)). Ogg
  plays through vorbisfile and WAV through SDL3. Output is no longer forced
  to 44.1 kHz.
- HRTF setting; the output device switches at once, and the device recovers
  when it is lost.
- Music, Effects, Voice and Ambience volume buses.
- Blocked sounds are muffled by a low-pass filter as well as made quieter.
- Room reverb crossfades between two effect slots; music fades are
  equal-power.
- Fixed: music stutter (stream buffers), an EFX worker race behind Windows
  startup crashes, one-shot sounds replaying after unpause, and voice
  stealing while finished sounds still held voices.

## Physics

- Closed 32-bit Newton 1.53 → open-source Newton Game Dynamics 2.36. Contact
  callbacks were rewritten; impact sounds, particles and `OnCollide` behave
  as before.
- Matching the original's behaviour: inertia is kept above 0.01 × mass, and
  contact normals from `NewtonCollisionCollide` are no longer reversed.
- Map objects placed slightly inside level geometry no longer stick. This
  covers sliders and doors, held levers, scripted bodies, pushed crates over
  hole edges, scaled bodies and stick areas that went past their pose.
- Fixed: crashes from compound-against-tree contacts (the ice cave), NaN
  bodies, and stick-area masses missing from saves.
- Remaining difference: Newton 2.36 has only world-wide sleep thresholds.

## Scripting

- AngelScript 2.7.1b → 2.38.0. The original game scripts run unchanged, and
  each `Run("...")` line is compiled once and cached.

## Modding and tools

- Addons: folders or `.zip` files in `addons/` override and add game files.
- glTF 2.0 models and maps next to Collada; `openhpl1-model` converts and
  checks them ([modding](docs/modding.md)).
- Developer mode: map reload while editing (F6, or on save in an addon
  folder), a level editor window (`--editor`), live control from
  `tools/levelview.py`, a debug overlay (F3) and a reference/shading panel
  (F2).
- Tests: engine, renderer and audio regressions, an automated smoke run
  through every map, reference-image capture and comparison, and generated
  physics, Dog and effects test maps.
