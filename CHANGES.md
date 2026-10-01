# Changes from the original

Everything Overture Remade changes compared to the code Frictional Games
released, i.e. FrictionalGames/HPL1Engine @ `5e441bb` and
FrictionalGames/PenumbraOverture @ `6a6953a` (imported unmodified in the first
commit), and the last official Linux build on Steam.

## Milestone 0: native 64-bit Linux build

### Build system and toolchain

- New top-level CMake build (C++17, Ninja friendly). It replaces the old
  per-project CMake files that linked against prebuilt 32-bit libraries in a
  `dependencies/` tree that was never published.
- Builds as 64-bit with current GCC/Clang. C++ fixes needed for that:
  - sort comparators (`operator()`) are `const`
  - no addresses taken of temporaries (Renderer2D shadow batching, Newton
    body matrix updates)
  - `SoundHandler::PlayStream` returned `false` as a pointer
- Third-party libraries vendored in `vendor/`:
  - Newton Game Dynamics 2.36, with ScummVM's includes replaced by
    `newton_compat.h`
  - AngelScript 2.38.0 and the `scriptstdstring` add-on
  - OALWrapper, ported to SDL3
  - glad 2 OpenGL loader (GL 4.6 compatibility plus the legacy extensions the
    engine probes)
- `run.sh` launcher.

### Windowing, input and platform (SDL 1.2 → SDL3)

- Window and OpenGL context creation rewritten for SDL3; a compatibility
  profile context is requested.
- A width/height of `-1` (or any value ≤ 0) means desktop resolution.
- Fullscreen at desktop size is borderless. Any other size switches the
  display mode (closest available mode).
- Opens on the largest monitor by default. Wayland has no "primary" monitor,
  so SDL picked whichever was listed first. `OVERTURE_DISPLAY` overrides it.
- Window size is read back in pixels after creation.
- Mouse:
  - SDL3 relative mouse mode replaces `SDL_WM_GrabInput`, giving raw relative
    motion
  - relative motion is summed from motion events instead of polled
  - the mouse wheel uses SDL3 wheel events, honouring flipped wheels
- Keyboard:
  - SDL3 keycodes
  - typed characters come from layout plus modifiers, replacing SDL 1.2's
    `unicode` field
  - the GUI key maps to the engine's META modifier
  - saved key bindings are unchanged
- The window close button and SIGTERM now quit the game (`SDL_EVENT_QUIT`
  used to be ignored).
- FLTK error dialogs replaced with SDL message boxes.
- SDL_ttf dropped. The engine only called `TTF_Init`/`TTF_Quit`; all game
  fonts are bitmap `.fnt` files. TrueType font loading now logs an error.
- Hardware gamma ramps removed (SDL3 has none). The gamma setting is applied
  by a shader pass instead (see "Gamma" below).

### Images and textures

- Every loaded image is converted to byte-ordered RGB24/RGBA32, and textures
  are uploaded based on channel count. The original guessed the channel
  order from the file extension (TGA meant BGR), which is wrong for SDL_image
  3. 8-bit palettised images stay single channel, as before.
- `gluBuild1D/2DMipmaps` replaced with `glGenerateMipmap`.
- Rectangle textures use clamp-to-edge. `GL_REPEAT` on them is invalid and
  made modern drivers drop the texture.
- Render-target textures no longer use GLX/WGL pbuffers. Nothing in the game
  used them; they are plain textures until FBO render targets exist.

### Rendering

- **Cg removed.** Shaders are GLSL, using ScummVM's ports of the original
  Cg programs:
  - new `cGLSLProgram` replaces `cCGProgram` and keeps the engine's existing
    shader interface unchanged
  - vertex and fragment programs are compiled as separable programs and
    combined in a program pipeline, matching Cg's model of independently bound
    stages
  - a request for `Foo_vp.cg` / `Foo_fp.cg` loads `hpl1_Foo.vertex` /
    `hpl1_Foo.fragment` from the shader directory
  - samplers `texN` are bound to texture unit N
  - matrix state parameters (e.g. `worldViewProj`) are computed from the GL
    matrix stacks, like `cgGLSetStateMatrixParameter`
  - a vertex program bound without a fragment program uses its matching
    "companion" fragment shader, because GLSL outputs cannot feed
    fixed-function fragment processing the way Cg vertex programs could
  - the pipeline is only bound while a stage is active; NVIDIA rejects draws
    with an empty pipeline instead of falling back to fixed function
- Shader changes compared to ScummVM's versions:
  - global aliases of built-ins (e.g. `vec4 position = gl_Vertex;`) became
    `#define`s, since non-constant global initialisers are not valid GLSL
  - shaders are compiled as `#version 330 compatibility`
- The Cg profile options (`ForceFP`/`ForceVP`) and the Cg profile probe at
  startup are removed.
- GLee replaced with glad; GLU no longer used.
- Alpha is forced to 1 before each buffer swap. The renderer uses destination
  alpha as scratch space, and Wayland compositors displayed the window as
  transparent (black screen).

### Physics (Newton 1.53 → Newton 2.36)

- Material contact callbacks rewritten for Newton 2: all contacts of a body
  pair arrive in one callback that walks the contact joint, replacing the
  begin/process/end callbacks. Impact and slide sounds, particle effects and
  `OnCollide` callbacks work as before.
- Collision primitives and compounds take a shape ID; `NewtonCreate()` takes
  no allocators; bodies are created with an initial matrix.
- Freeze/sleep API: `SetEnabled` uses `NewtonBodySetFreezeState`,
  `GetEnabled` uses `NewtonBodyGetSleepState`, auto-disable uses auto-sleep.
- `NewtonInitGlobals`/`NewtonDestroyGlobals` called around physics use.
- Debug geometry drawn with `NewtonCollisionForEachPolygonDo`.
- Fixed in the vendored Newton: `NewtonCollisionCollide` returned reversed
  contact normals for convex-convex pairs (it read the mass of temporary
  bodies that was never set). Characters were pushed into loose objects
  instead of out of them, which could register as a fall and kill the player.
- Each axis of a body's inertia is kept above 0.01 × mass, as Newton 1.53 did
  (Newton 2 allows 0.001 × mass). Without it, thin objects such as bottles
  spin up while held.
- **Behaviour differences:**
  - Newton 2.36 has only world-wide sleep thresholds, so per-body auto-disable
    thresholds are stored but have no effect
  - static level meshes (tree collisions) are built unoptimised, as in
    ScummVM's port, because Newton 2's optimiser breaks level geometry

### Scripting (AngelScript 2.7.1b → 2.38.0)

- Modules and function objects replace the old engine-level API
  (`GetFunctionIDByName`, `Discard`, ...).
- `Run("Func()")` used AngelScript 2.7's removed `ExecuteString`. It now
  compiles the line as a module function, and each distinct line is compiled
  once and cached; per-frame calls such as `OnUpdate()` used to be recompiled
  every frame.
- All script functions use the generic calling convention, and they are
  registered as `asGENFUNC_t` (required by AngelScript 2.38).
- `bool` arguments and return values are 1 byte (AngelScript 2.38).
- The engine's own `string` registrations (`stdstring.cpp`,
  `scriptstring.cpp`) are replaced by the standard `scriptstdstring` add-on.
- The original game scripts (`.hps`) run unchanged.

### Audio

- If the saved OpenAL device does not exist (e.g. "PulseAudio Software" from
  the old bundled OpenAL), the system default device is used.
- Music stutter fixed: streaming buffers are 8 × 16 KB (was 8 × 4 KB, about
  185 ms) and are refilled at least 50 times a second (was 10).
- OALWrapper threads and mutexes use SDL3.

### Game

- Settings, saves and logs live in
  `~/.frictionalgames/Penumbra/OvertureRemade/` instead of `.../Overture/`,
  so the original game's files are never touched.
- GLSL shaders are loaded from `<executable dir>/shaders/` or
  `OVERTURE_SHADER_DIR`.

### Debugging aids

- `OVERTURE_GL_DEBUG=1`: debug GL context; KHR_debug messages go to `hpl.log`
  (each message at most 3 times).
- `OVERTURE_DUMP_FRAMES` / `OVERTURE_DUMP_DIR`: save chosen frames as PNG.
- The pipeline validation log is written when GPU program debug info is on.
- `OVERTURE_SMOKE`: automated run through every map (see README).
- `OVERTURE_MAP=name[:start]`: load a map at startup, skipping the menus.
- `OVERTURE_PHYSLOG=1`: physics event log in `hpl.log` (`PHYS:` lines).
- Physics test map, generated by `testmap/generate.py` and loaded from
  `<executable dir>/testmap/`. Resource directories given as absolute paths
  keep their case.

## Milestone 2: widescreen and quality of life

### Widescreen 2D/HUD

- The 2D layer keeps its 800×600 layout but is no longer stretched to the
  window. One virtual unit is the same number of pixels on both axes and the
  layout is centred; in a window that isn't 4:3 the screen reaches past it,
  from `-offset` to `size + offset` (`iLowLevelGraphics::GetVirtualOffset`,
  `SetVirtualOrthoProjection`). 4:3 windows look as before.
- Menus, inventory, notebook, HUD and text therefore keep their proportions
  instead of being 33% wider at 16:9.
- Full-screen overlays cover the whole window: fades, cinematic bars, damage
  and hit splashes, death fade, flash, underwater tint, save flash, in-shadow
  vignette, noise filter, inventory/notebook/panel/death-menu dimming, map
  load background and loading screen.
- Mouse position and movement use the same uniform scale. The menu cursor can
  reach the real screen edges. Horizontal mouse movement used to be scaled by
  800/width, so at the same height a wider window made turning slower (by 25%
  at 16:9 compared to 4:3). Turning and cursor speed per pixel now match a
  4:3 window of the same height; vertical movement is unchanged.
- `cCamera3D::UnProject` and the grab crosshair projection account for the
  offset, so picking under an off-centre crosshair matches what is drawn.
- Pre-menu rain falls across the whole width.
- Not changed: the interact-mode crosshair keeps its 4:3 movement area (the
  same view angles as the original), menu and logo art is shown centred with
  black borders, and the unused GUI set, 2D camera, tile map and haptics code
  still assume a stretched canvas.

### Fonts

- Text is drawn from TrueType fonts, rendered with FreeType at the size it
  has on screen and placed on whole pixels, instead of scaling the game's
  bitmap fonts. It is sharp at any resolution.
- The fonts are in `fonts/` (copied to `<executable dir>/fonts/`): Arimo for
  the menu and game text, Cousine for the computer text. `fonts/fonts.cfg`
  says which file replaces which of the game's `.fnt` fonts, and at what size
  and width. These are set so that capital letters are as tall as in the
  original font and the game's English text is as long, so text sits where it
  did and wraps at about the same places. Line height and baseline still come
  from the game's `.fnt` file, and its bitmaps are used for characters the
  font file lacks.
- The layout does not depend on the resolution: advances are the font's own,
  unhinted.
- The black outline of messages, subtitles and the intro text is the glyph's
  contour widened with a round pen (`eFontEffect_Outline`), and the glow of
  hovered menu buttons is the glyph blurred (`eFontEffect_Glow`). Both used
  to be copies of the text moved one to three units to the sides. Drop
  shadows and moving copies (main menu buttons, "click to continue", pre-menu
  text) are unchanged.
- The credits are not snapped to whole pixels vertically, so that they scroll
  evenly.
- `OVERTURE_BITMAP_FONTS=1`, or a missing `fonts/` directory, gives the
  bitmap fonts and the old outline and glow.
- Rendered glyphs are kept for as long as the font is loaded; nothing is
  dropped when sizes stop being used.
- Fixed in passing: creating or reloading a texture left it bound in place of
  the texture the engine had set on that unit, and a destroyed texture stayed
  recorded as set. With only text on screen (pre-menu) the glyphs were drawn
  untextured.

### Gamma

- The gamma setting works again. Before a frame is shown it is copied to a
  texture and drawn back through `out = in ^ (1 / gamma)`, the curve
  `SDL_SetGamma` loaded into the display's gamma ramp
  (`cLowLevelGraphicsSDL::DrawGammaPass`, `hpl1_gamma_correction` shaders).
  At 1.0 the pass is skipped.
- It affects only the game's window, also when windowed, and no longer
  changes the rest of the desktop or stays behind after a crash. Frame dumps
  include it.
- The frame has 8 bits per channel before the pass, so raising gamma a lot
  spreads the dark levels apart (banding in dark areas), as it did with a
  ramp on an 8-bit display.

### Field of view

- "Field of View" in the graphics options: the vertical angle, 60 to 100
  degrees in steps of 5, 70 being the original (`FOV` under `Graphics` in
  `settings.cfg`, which takes any value from 30 to 140). The view is Hor+, so
  at 16:9 the default is about 102 degrees horizontally (86 at 4:3).
- The game keeps setting the angles it was written with (70 normally, 78
  when hidden, the pulsing when an enemy is close, the zoom of the save
  effect); the camera multiplies the tangent of the half angle by a factor
  (`cCamera3D::SetFOVScale`), so these effects keep their proportions.
  The check for an enemy being in view when hiding still uses the unscaled
  angle, as gameplay.
- The label and tip of the option are not in the game's language files and
  are always in English.
- The bounding box used for fog culling is computed from the real corner of
  the view. The old formula used `fov * aspect / 2` as the horizontal half
  angle, which reaches 90 degrees with a wide window and a large angle.

### Display options

- "Resolution" in the graphics options steps through "Desktop" (the
  desktop's size, stored as -1 x -1), the fullscreen modes of the display
  and the sizes of the original list that fit on it. It used to offer a
  fixed list up to 1920x1200 and showed the desktop setting as "-1 x -1".
- New "Fullscreen" option (`FullScreen` under `Screen` in `settings.cfg`,
  which had no menu entry).
- New "Display" option: "Automatic" (the largest monitor, as before) or one
  of the connected monitors (`Display` under `Screen`, -1 or an index from
  0). `OVERTURE_DISPLAY` still goes before it.
- All three take effect at the next start, like the resolution did; the menu
  shows the game's restart notice. V-Sync was already in the advanced options
  and applies at once.
- `iLowLevelGraphics` lists displays and their modes (`GetDisplayNum`,
  `GetDisplayName`, `GetDisplaySize`, `GetDisplayModes`, `GetDisplay`), and
  `Init` takes the display to open on.
- The labels and tips of the new options, and "Desktop" and "Automatic", are
  not in the game's language files and are always in English.

## Known issues

- Intermittent heap corruption on exit (`free(): corrupted unsorted chunks`).
- Physics may feel slightly different from the original (see above); not yet
  compared side by side.
- Mouse sensitivity scales with resolution (original behaviour).
- Windows build not set up yet.
