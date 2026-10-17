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

- Window and OpenGL context creation rewritten for SDL3. (A compatibility
  profile context at first; a core profile one since "No fixed-function,
  core profile" below.)
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
  - the pipeline was only bound while a stage was active (NVIDIA rejects
    draws with an empty pipeline instead of falling back to fixed
    function); it now always has both stages, see "No fixed-function, core
    profile" below
- Shader changes compared to ScummVM's versions:
  - global aliases of built-ins (e.g. `vec4 position = gl_Vertex;`) became
    `#define`s, since non-constant global initialisers are not valid GLSL
  - shaders are compiled as `#version 330` (compatibility at first, now
    core)
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
  the menu and game text, Recursive Mono Casual Italic for the text on paper
  (notebook notes and tasks), Departure Mono for the computer text.
  `fonts/fonts.cfg` says which file replaces which of the game's `.fnt` fonts,
  and at what size and width. These are set so that capital letters are as
  tall as in the original font and the game's English text is as long, so text
  sits where it did and wraps at about the same places. Line height and
  baseline still come from the game's `.fnt` file, and its bitmaps are used
  for characters the font file lacks.
- The layout does not depend on the resolution: advances are the font's own,
  unhinted.
- The computer text (pre-menu, intro and map load texts) is a pixel font
  (`Pixels` in `fonts.cfg`). Each of its pixels is a whole number of screen
  pixels, the most that fit in the size the text is given, and the glyphs
  advance by whole font pixels, so the pixels are sharp, square and on one
  grid. The text is at most its laid out size and length, and smaller where
  the size falls between two whole scales: at 720p a font pixel is one screen
  pixel and the letters are small and wide apart.
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

### Mouse sensitivity

- Turning no longer depends on the resolution. Mouse movement reaches the
  game in virtual units, 600 / window height per mouse count, so a higher
  window turned slower at the same sensitivity setting. Look input is now
  scaled by window height / 1440 (`kLookReferenceHeight`): at any size the
  player turns as fast as in a 1440-pixel-high window before, and faster
  than before in a lower one (2.4 times at 600). Everything driven by look
  input follows: turning, the interact-mode crosshair, moving and turning
  held objects.
- Menu, inventory and notebook cursors are unchanged and move one pixel per
  mouse count.
- With the mouse grabbed, SDL3 reports unaccelerated movement (the desktop's
  pointer speed and acceleration are not applied), which is its default.
- Pitch is still 4/3 as fast as yaw for the same movement, as in the
  original.

### Frame rate above 60

- "Limit to 60 FPS" in the advanced graphics options (the `LimitFPS`
  setting, which had no menu entry; on by default, as before). On: a frame
  is drawn after each logic step, as in the original. Off: frames are drawn
  as fast as possible, or at the display's rate with V-Sync on. It applies
  at once.
- The game logic and physics still run at 60 steps per second. With the
  limit off each frame shows the world between the last two steps
  (`cGame::Run`, `cScene::StoreInterpolation` / `ApplyInterpolation` /
  `RestoreInterpolation`): the camera, everything attached to a physics body
  and the entities of character bodies (enemies) are placed between their
  positions before and after the last step for drawing and put back
  afterwards. A move of more than 2 m in one step counts as a teleport and
  is not blended. Without this, frames between two steps were copies of
  each other.
- Looking around does not lag behind the mouse (`cButtonHandler::
  OnInterpolate`): the part of the last step's turn that came from the mouse
  is shown in full, and movement that has arrived since is added
  (`iMouse::GetPendingRelPosition`). Turns that do not come from the mouse
  are blended.
- Held items and hands follow the camera of the frame
  (`cPlayerHands::OnInterpolate`).
- Cursors move every frame too: the main menu's cursor, and the crosshair
  that is the cursor of the inventory, notebook, panels, death menu and
  interact mode, are drawn ahead of their position by the mouse movement
  that has arrived since the last step (`cButtonHandler::
  GetCursorDrawOffset`), and so is an item being dragged in the inventory.
  What is under the cursor (highlights, clicks) is still decided 60 times
  per second.
- Not blended, so still moving 60 times per second: skeletal animations,
  particles, the sway of held items, lights and entities that scripts move
  without physics. The image trail effect
  (enemy close, save) fades per frame and so gets shorter at higher frame
  rates, as it already did with `LimitFPS` off.
- With the limit and V-Sync both off nothing caps the frame rate.
- The logic timer and the frame time use a nanosecond clock
  (`GetApplicationTimeExact`) instead of whole milliseconds.
- The advanced graphics menu has one more row; its rows are 26 units apart
  instead of 29.

## Milestone 3: renderer foundation

### Reference images

- F2 (or `OVERTURE_REFVIEW=1`) opens a Dear ImGui panel (`cRefView`) that
  goes to the places of the reference images by hand: it lists the maps and
  the loaded map's places and directions, loads maps (from the main menu
  too), and sets the view up as in the frames. For this the debug overlay
  can draw windows of the game's (`iLowLevelGraphics::SetDebugPanel`) and
  take the mouse and keyboard (`SetDebugPanelInput`): the pointer is freed
  and drawn by ImGui, and the game gets no mouse input and no key presses
  but F1-F12 until the panel is closed.
- `OVERTURE_REFSHOTS` saves frames from fixed places in every map and quits
  (`cRefShots`); `tools/refshots.py` runs it headless and compares two runs.
  To make two runs give the same frames:
  - the logic timer has a fixed-step mode (`cLogicTimer::SetFixedStep`):
    one logic step per drawn frame whatever the clock says, and the frame
    time handed to the renderer is the step size
  - the random seed is set when the run starts and at each map
  - sounds are chosen with a random number generator of their own
    (`cMath::SoundRandRectl/f`): they are chosen when others end, which
    happens in real time, and used to shift the random numbers of
    particles and flickering lights
  - radio subtitles are off and flickering lights are held on
- The run turns off the game's 2D drawing (`cGraphicsDrawer::SetSkipDrawing`)
  and makes the player unhurtable (`cPlayer::SetGodMode`); it keeps the
  flashlight on and powered. It restores the frame-rate limit and subtitle
  settings before the game saves them.
- `iLowLevelGraphics::SaveNextFrameToPNG` saves the next frame as it is shown,
  after the gamma pass. `OVERTURE_DUMP_FRAMES` uses the same code and now
  writes RGB images instead of RGBA.

### Fallbacks for old GPUs removed

The game requires OpenGL 4.1, so the paths for GPUs without what GL 3.3
guarantees were never taken. Removed, with the reference images unchanged:

- the `Fallback01`/`Fallback02` lighting materials and their shaders, and
  the flat unlit fallback for lit materials (`Flat` stays as its own material
  type)
- the Shader Quality option (Very Low/Low/Medium picked those fallbacks) and
  its `ShaderQuality` setting; Graphics options now end at Field of View
- the two-pass spotlight for GPUs with four texture units or fewer, and its
  `_pass1`/`_pass2` shaders
- the post-effect blur fallback and the static-loop motion blur shader
- the vertex-array fallback for GPUs without vertex buffer objects
  (`cVertexBufferOGL`)
- single-sided stencil shadows: shadow volumes always use two-sided stencil,
  now through GL 2.0's `glStencilFuncSeparate`/`glStencilOpSeparate` instead
  of `EXT_stencil_two_side`/`ATI_separate_stencil`
- capability checks for features of GL 3.3 or older: texture rectangles,
  fragment programs, multitexturing, multisampling, separate blend functions;
  the startup support check and its error messages; the NV register combiner
  and ATI fragment shader caps

### Clean GL debug output

A run of every reference view with `OVERTURE_GL_DEBUG=1` logs no driver
messages (NVIDIA 615). Fixed for that:

- binding a texture to a unit that had none called `glDisable(0)`
  (`GL_INVALID_ENUM`, from font and HUD drawing among others)
- the debug log repeats a message at most three times by its text, not by
  its id: NVIDIA gives every `GL_INVALID_ENUM` the same id
- NVIDIA's "vertex shader is being recompiled based on GL state" note
  (id 131218) is not logged

### Off-screen frame

- The frame is drawn into a framebuffer object with colour and
  depth/stencil textures, and copied to the window when it is shown. The
  window has no depth, stencil or multisample buffer of its own.
- Multisampling (FSAA) uses multisampled textures. Copies of the screen
  into textures (post effects, refraction) resolve the copied area first:
  copying from a multisampled buffer is a GL error, which the window's
  buffer used to get away with.
- The gamma pass reads the frame texture instead of copying the frame.
- Two framebuffers: the 3D scene and its post effects are drawn into the
  scene framebuffer (`BeginScene`/`EndScene` in `cScene::Render`), which is
  then copied to an output framebuffer of the window's size; the HUD and
  menus are drawn there, and the gamma pass copies that to the window.
  `GetRenderSize()` is the scene's size, used by the 3D renderer, the post
  effects and the light clip rectangles; `GetScreenSize()` stays the
  window's.
- Render Scale (Advanced Graphics, `RenderScale` in `settings.cfg`): the
  3D scene is drawn at 50, 75, 100, 150 or 200% of the window size and
  scaled to it (bilinear) before the HUD, which stays at the window's
  resolution. Applies at once. Advanced Graphics rows are 24 units apart
  instead of 26 to make room for it.
- The scene framebuffer is 16-bit floating point (`GL_RGBA16F`), so the
  light passes add up without being rounded to 8 bits after each one, and
  their sum can go past 1 until the scene is converted. Each draw's colour
  is still clamped to 0..1 before blending (`GL_CLAMP_FRAGMENT_COLOR`), as
  with an 8-bit buffer. `EndScene` converts the scene to the 8-bit output
  with a shader (`hpl1_output`) that adds a fixed half-step noise pattern
  (dithering) against banding, and does the render-scale filtering.
  Reference images: about 190 of 552 frames differ by more than 2 levels,
  most by 10 or less. Dark scenes come out up to about 3 levels brighter
  on average: light contributions smaller than one 8-bit step were rounded
  away before. A few hundred pixels per frame differ more in overexposed
  spots, where light adds up past 1 before a blend multiplies by it.
- The gamma pass and the output pass share one full-screen pass helper.
- `SetRenderTarget` renders into a texture through a framebuffer object
  (it did nothing since the pbuffer code was removed);
  `RenderTargetHasZBuffer` and `FlushRenderTarget` are gone.
- The post-effect blur (bloom, depth of field) renders its two passes into
  textures. It drew them into the top-left corner of the frame and copied
  them out, and with depth of field and no sky box the corner was left
  showing a small blurred copy of the picture (e.g. at the mine entrance).
- F12 screenshots (`SaveScreenToBMP`) read the last finished frame from the
  frame texture, before the gamma pass; they read the back buffer after the
  swap before, whose contents GL does not define.
- Reference images: unchanged except for ±1 level rounding noise in about
  400 of 552 frames, which blending into an FBO texture gives on NVIDIA,
  and a few dozen pixels on three frames. With FSAA 4 the edges differ
  (other sample positions) and are still smoothed.

### Shader system

- Shader files can `#include "file"` from the shader directory (each file
  once). Every file gets its own GLSL source number, and a compile error
  lists which number is which file, so `2(16)` reads as line 16 of the
  second file.
- Programs can be variants of one file compiled with `#define`s
  (`kShaderVariants` in `GLSLProgram.cpp`). The ten light fragment programs
  and four light vertex programs are now `hpl1_light.vertex`/`.fragment`
  (`SPOT`, `SPECULAR`, `NORMAL_MAP`, `SPECULAR_MAP`) with their shared
  varyings in `hpl1_light.glsl`; the three transparent fog programs are
  `hpl1_Fog_Trans.fragment` (`MOD`, `MODX2`) and the two blur programs
  `hpl1_PostEffect_Blur.fragment` (`RECT`). Light passes write 0 alpha in
  every variant; some left it undefined.
- Removed: the per-combination files the variants replace; the `BumpSpec2D`
  material type and its shaders (no game material uses it, and its vertex
  shader did not compile); `hpl1_Diffuse_ColorMul.vertex` and
  `hpl1_PostEffect_Offset`, which nothing loaded.
- The model-view, projection and texture matrices are kept on the CPU by
  the low-level graphics (`GetMatrix`) and loaded into GL from there.
  Programs take `worldViewProj` and the other state matrices from those
  instead of reading them back from GL (`glGetFloatv`) on every draw.
  `RotateMatrix`, which nothing called (and which turned by 1 degree
  whatever it was given), is gone.
- Per-frame values are in one uniform block, `hpl_Frame` in
  `hpl1_frame.glsl`: view, projection and view-projection matrices, camera
  position, 3D render size, near/far planes and render time. The 3D
  renderer sets it once per frame (`SetFrameUniforms`), and every program
  that declares it reads it from uniform buffer binding 0. Depth of field
  and the special refraction read the render size from it instead of a
  `screenSize` uniform set per program; two refraction shaders declared
  `screenSize` without using it. Per-object values such as
  `worldViewProj` stay program uniforms.
- `OVERTURE_SHADER_RELOAD=1` recompiles shaders while the game runs: twice
  a second the files each program was compiled from (with its includes)
  are checked, and programs with a changed file are compiled again. One
  that does not compile keeps its old version and logs the error. The
  gamma and output passes are created again after any change. Uniform
  values set once before a reload are lost until set again; most are set
  for every draw.

### GPU profiling

- Dear ImGui 1.92.9b is vendored (`vendor/imgui`; the OpenGL 3 backend is
  built on glad through `vendor/imgui_glad`). F3 (or
  `OVERTURE_DEBUG_OVERLAY=1`) shows an overlay over the finished frame, in
  Cousine at a size for the window's height: frame time, GPU frame time,
  draw calls and triangles, scene size, and the GPU time of each section of
  the frame. It takes no input (the reference view panel does, see
  Reference images) and writes no `imgui.ini`.
- Sections (`BeginGpuSection`/`EndGpuSection`, `cGpuSection`) mark the
  frame: World (Z, occlusion queries, lights, diffuse, fog, sky box,
  transparent, debug), post scene draw, post effects (depth of field, motion
  blur, bloom, image trail), scene output, GUI and present. They are always
  GL debug groups, which RenderDoc and the driver's debug output show;
  while the overlay is shown they are also timed with `GL_TIMESTAMP`
  queries, read four frames later if the GPU has them, so timing never
  waits for the GPU (`cGpuProfiler`).
- With `OVERTURE_GPU_LOG=<file>` the sections are always timed and every
  frame's times are written to that file, one line a frame; a frame that
  is saved as a picture carries the picture's name.
  `docs/gpu-profiler-run1.md` is a first run of it over the reference
  viewpoints.
- Draw calls and triangles are counted at every `glDrawElements` and
  immediate-mode draw.
- GL objects carry names (`glObjectLabel`, `cGpuProfiler::Label`) for
  RenderDoc and the debug output: textures by their resource name, the
  scene, resolve, output and render-target framebuffers and their
  attachments, programs by shader file and variant, program pipelines and
  the `hpl_Frame` buffer. Vertex buffers are not named.
- Occlusion queries no longer wait for the GPU. After each frame the
  renderer spun until every query of that frame had its result; now each
  query cycles through three GL queries and takes the newest result that
  is ready, usually the previous frame's. Only light halos use them (their
  alpha is the share of their samples that pass the depth test), so a halo
  fades a frame later than before. Reference images: 4 of 552 frames
  change, all with halos; three by a few hundred to ten thousand pixels,
  and the boat cabin's swinging paraffin lamp, whose halo feeds bloom over
  the whole frame (waiting for the current result gives the old frame
  back).
- Reference images: unchanged.

### No fixed-function, core profile

The game now asks for an OpenGL core profile context (4.6, 4.5, 4.3, 4.1 or
3.3, the first the driver gives), so nothing of the fixed-function pipeline
is left. RenderDoc needs a core profile; capturing with it is untested.

- **2D reference images.** After the maps, the reference run loads the
  first map again and saves the HUD, inventory, notebook (cover and task
  list), keypad, a message and four main menu pages as `2d-*.png`. The
  menu's snow is seeded before the menu is created, so they repeat.
- **Shaders for every draw** (`cFixedFunctionShaders`). The HUD, fonts,
  menus, particles, billboards, beams, transparent materials without fog,
  the Flat material, debug lines and some post-effect passes were drawn
  without programs. They now get a vertex stage that transforms by the
  engine's own matrices and passes the colour and texture coordinates on,
  and a fragment stage generated for the texture environment the engine has
  set: per unit the texture target, `GL_MODULATE` or the `GL_COMBINE`
  settings of `SetTextureEnv`, and the constant colour. A stage is compiled
  the first time its combination is drawn.
  `SetTextureEnv` and `SetTextureConstantColor` only record state.
- `cGLSLProgram::Bind`/`UnBind` only take note of the program. Before every
  draw `cLowLevelGraphicsSDL::PrepareDraw` puts the bound programs, or the
  fixed-function stages where there is none, in the program pipeline, which
  stays bound.
- **Alpha test and clamping in the fragment shaders.** Every fragment
  shader is compiled with an epilogue: the shader's `main` is renamed and
  called by a new `main` that clamps the colour to 0..1 (what
  `GL_CLAMP_FRAGMENT_COLOR` did) and discards by the alpha test settings,
  which are in a uniform block (`hpl_State`, binding 1) written by
  `SetAlphaTestActive`/`SetAlphaTestFunc`.
- Clip planes go to `gl_ClipDistance` from the fixed-function vertex stage.
  Nothing uses them (the GUI code that did is commented out), and engine
  vertex programs do not write clip distances.
- The texture matrix stack is no longer applied to anything; nothing set it.
- **Vertex attributes.** Vertex shaders read `hpl_Vertex`, `hpl_Normal`,
  `hpl_Color` and `hpl_MultiTexCoord0..4`, declared by the preamble at fixed
  locations (0, 1, 2, 4..8), instead of `gl_Vertex` and the like.
- **Vertex array objects.** Each vertex buffer has one, made when first
  drawn. `DrawQuad`, `DrawLine` and the other immediate-mode calls, and the
  2D batch, copy their vertices to a 4 MB stream buffer that is replaced
  when full; indices given as memory (shadow volumes, the batch) go to a
  stream index buffer the same way. Every draw binds its vertex array.
- The colour and texture coordinates that `glColor` and `glTexCoord` left
  current are kept by the low-level graphics and given to attributes
  without an array, so draws that relied on them get the same values.
- Quads are drawn as two triangles (0,1,2 and 0,2,3): single quads as a
  fan, the batch and quad vertex buffers through triangle indices.
- The gamma and output passes make their triangle from `gl_VertexID`.
- Textures of one and two channels (fog, attenuation) are `GL_R8`/`GL_RG8`
  with a swizzle that reads them as luminance and luminance-alpha did.
  `GL_CLAMP` wrapping is `GL_CLAMP_TO_EDGE`.
- Shaders are `#version 330 core`; `texture1D`, `texture2DRect` and
  `textureCube` became `texture`.
- Gone: `glBegin`, client-side arrays, the GL matrix stacks, `glTexEnv`,
  `glAlphaFunc`, `glClipPlane`, `glColor`, texture enables,
  `glPushAttrib` (the fullscreen passes save what they change),
  `glShadeModel`, the perspective hint, and the ARB-suffixed buffer calls.
- Reference images, against the frames before this section: 557 of 562 the
  same. About half of those differ by one or two levels in places, since
  the driver's own fixed-function shaders rounded differently. Five frames
  have single pixels that changed at the edges of alpha-tested fences and
  grates, where a fragment's alpha is right at the threshold.
- Checked on Mesa's software renderer (llvmpipe, OpenGL 4.6 core): no GL
  errors on the test map's reference views, and 72 of the 74 frames are
  within 12 levels of NVIDIA's (the other two differ in about 2000 pixels
  each, not looked into).

## Milestone 4: lighting, shadows and effects

### Linear lighting (option, off by default)

`docs/colour-space-audit.md` describes what the renderer and the game data
do with colour, and what follows for this option.

- **Linear Lighting** in Advanced Graphics (`LinearLighting` in
  `settings.cfg`), applied at once. Off, nothing changes: the reference
  images match the ones before, and the HUD and menus are the same either
  way.
- On, the scene buffer holds values proportional to light. Every draw into
  it still computes the colour the original shaders compute, and the shared
  end of the fragment shaders converts that colour (power 2.2) before it is
  blended. The output pass converts the finished scene back for the screen.
  So each surface, light, particle and overlay contributes what it always
  did, and what changes is how they add up: light passes on each other and
  on the ambient pass, additive halos and particles, alpha blending and
  fog, anti-aliasing, blur and render-scale filtering.
- Textures are not loaded in sRGB formats, against the plan. Converting in
  the shader keeps one copy of each texture for the scene and the HUD, lets
  the option switch while playing, and makes "the same colour as before"
  exact for every multiplication. The cost is that texture filtering stays
  on screen values, as it always was.
- The light shader converts its diffuse and specular terms separately and
  adds them as light. N·L and the falloff ramp, which were made by eye on
  screen values, are raised to `kAmountExp` (2.2 in `hpl1_light.fragment`):
  2.2 gives a single light its original look, 1.0 would treat them as
  amounts of light. N·L is clamped at 0 there; a surface facing away from
  the light loses specular to the negative diffuse term as before.
- Modulate ×2 materials (destination × source × 2) get the factor 2^2.2
  that their doubling means on linear values; the low-level graphics passes
  the extra to the shaders when that blend function is set.
- The colour is not clamped at 1 by shaders that work on linear values (the
  light passes, depth of field, motion blur, blur, refraction); the rest
  clamp as before.
- Post effects: with the option on, the screen copies and blur textures are
  16-bit float (8 bits of linear values band in the dark). Bloom and the
  water refraction convert to screen values for their own arithmetic and
  back; the image trail and the sky part of depth of field draw the copies
  unconverted (`SetLinearSource`).
- Six values tune the look (`cLinearLightingTuning`), as sliders under
  Renderer in the F2 panel, applied at once and kept in `settings.cfg`:
  - Light power (`LinearLightExp`, 1 to 2.2): the power for N·L and the
    falloff ramp described above, which was the constant `kAmountExp`.
  - Exposure (`LinearExposure`): scales the finished scene's light in the
    output pass. It brightens places lit by one light too, which the
    conversion alone leaves as they were.
  - Glow (`LinearGlow`): scales the light of what is drawn with an additive
    blend and is not a light pass: halos, illumination maps, additive
    materials and particles. They can be brighter than white this way. The
    scale is in proportion to the brightness of what is drawn: white gets
    all of it, the faint edge of a halo almost none.
  - Blend match (`LinearBlendMatch`, 0 to 1): alpha blending, below.
  - Shoulder and White (`LinearShoulder`, `LinearWhite`): tonemapping,
    below.

  At 2.2, 1, 1, blend match 0 and shoulder 1 nothing is tuned. The
  defaults, chosen by playing, are light power 1.8, exposure 1.3, glow
  1.45 and blend match 1, with tonemapping off (shoulder 1). On the
  reference viewpoints the average brightness is 32 with the option off, 27
  with it on and untuned, and 33 with the defaults and the soft bloom.
- **Alpha blending.** On linear values the same alpha covers more when
  something light is blended over something dark: smoke, steam and snow
  particles were much more opaque than without the option, and fog was
  heavier and showed the steps of its 8-bit alpha as bands on dark
  surfaces (the outside map). With blend match at 1, what is drawn with an
  alpha blend into the linear scene gets the alpha that gives the original
  result over a backdrop of screen value 0.15 (in the code appended to the
  fragment shaders; `hpl_AlphaTest.z`). Exact over that backdrop, close
  over others, and it goes the right way for dark over light too.
- **Tonemapping**, in the output pass after the exposure, only with linear
  lighting. Light up to the shoulder (set as a screen value, 0 to 1) is not
  changed, so the dark and middle parts of the picture stay as they are.
  Above it each channel follows Reinhard's curve with a white point: it
  leaves the shoulder at the same slope and reaches white at White times
  the light of white. Bright colours go towards white as they do when they
  clip, but gradually. Shoulder 1 is no tonemapping: values clip at white
  as before.
- For bright values to reach the output pass, bloom no longer clamps the
  scene at white (it converts for itself now, and its blur is cut at white
  as the 8-bit one was), and neither do the copies of the scene drawn back
  into it (image trail, the sky part of depth of field; `hpl_ColorSpace.w`).
  The water refraction still does, under water surfaces.
- State for this is in the `hpl_State` uniform block next to the alpha
  test (`hpl_ColorSpace`), which every program the renderer draws with now
  declares, together with `hpl_ToLinear()` and `hpl_ToEncoded()`.
- What it looks like so far (reference views, on against off): the same in
  places lit by one light, darker where lights, the flashlight, specular
  and ambient pile up (those used to clip to white), and additive glows
  are much weaker: the boat cabin's windows no longer burn out to white.
  Average brightness of the changed frames drops by about a sixth. Nothing
  is retuned yet: no exposure, tonemapping or brighter-than-white emitters.
- Checked: all 562 reference frames with the option on (no shader or GL
  errors; three maps also with a debug context, 4x anti-aliasing and motion
  blur), and with it off against `main` (same; the test map's frames differ
  in a full run because its loose objects settle differently, and match
  when it is captured alone).

### Soft bloom (option)

- **Bloom** in the graphics options is now Off, Original or Soft
  (`BloomType` in `settings.cfg`: 0 original, 1 soft; `Bloom` still switches
  it on and off). Original is the game's bloom and the default.
- Soft (`cRendererPostEffects::RenderBloomChain`,
  `hpl1_PostEffect_BloomChain`): the part of the scene brighter than a
  threshold, with a soft start below it, is drawn at half size, halved
  again and again down to a level 6 to 11 pixels high, and the levels are
  blended back up, each with the wider ones below it. The result is added to
  the scene. Because the chain ends at a fixed size, the glow covers the
  same part of the picture at every resolution and render scale, which the
  original's does not (compared at 1280x720 and 2560x1440).
- It works with and without linear lighting; its textures are 16-bit float
  and hold linear values either way. With linear lighting, what is brighter
  than white (the glow value, lights adding up) blooms more than white does.
- Three values tune it (`cBloomTuning`), as sliders in the F2 panel, kept in
  `settings.cfg`: Threshold (`BloomThreshold`, a screen value, 0.3),
  Intensity (`BloomIntensity`, 1.55) and Spread (`BloomScatter`, 0.55: the
  share each level takes from the wider one below). The defaults were
  chosen by playing indoors with linear lighting; on the outside map most
  of the picture is above the threshold.
- A program that the game data has no file for is now loaded by its name
  (`cGpuProgramManager::CreateProgram`): the shaders come from the shader
  directory, and new ones have no counterpart in the game data.
- Fragment shaders that define `HPL_NO_CLAMP` are not clamped at 1 whatever
  the lighting.
- The F2 panel's sections fold; Viewpoints and the reference image options
  start folded.

### Shadow maps (option)

- **Shadows** in Advanced Graphics has two more settings, Soft and Soft, Only
  Static (`ShadowType` in `settings.cfg`: 0 the original shadow volumes, 1
  shadow maps; `Shadows` is as before: all, only static, none). The original
  shadows are the default and are drawn as they were.
- With shadow maps, what a light sees is drawn into a depth texture before
  its light passes (`cRenderer3D::RenderShadowMap`), and the light shader
  looks every lit point up in it (`hpl1_shadow.glsl`). A spotlight has one
  map, drawn with its own view and projection. A point light has a cube map:
  six views of 90 degrees, of which only those are drawn whose part of the
  light's reach can be in the picture. There is one texture of each kind,
  used by one light after the other, so every light with shadows draws its
  map again in every frame.
- The same things cast shadows as before: what the maps and entities mark as
  casting, collected per light as for the volumes. Two differences. A
  one-sided wall seen from behind casts a shadow (the volumes are made from
  the sides that face the light only); "Sides drawn" in the panel has the
  original rule. And materials with holes cut by their alpha (fences,
  gratings), which the volumes leave out, cast shadows with the holes.
- Edges, as `ShadowFilter`:
  - 0, hard: one sample, filtered between the map's texels.
  - 1, soft: blurred by the same amount everywhere (`ShadowSoftness`).
  - 2, soft by distance, the default: for every point, what is between it
    and the light is looked for first, and the edge is blurred by how far
    behind that the point is, for a light of radius `ShadowLightSize` (5 cm).
    A shadow is sharp where it starts and widens with the distance, up to
    `ShadowMaxBlur` (3 cm). The limit is low on purpose: what is right next
    to a light, like the cage of a lamp, has a very wide edge by this rule,
    and at 12 cm the cages' shadows across the caves were blurred away.

  Blurs are radii at 1 m from the light and grow with the distance. The
  samples are spread over a disc that faces the light and turned by a
  different angle in every pixel (`ShadowNoise`).
- A sample is compared with the nearer of two distances: its own, and that
  of the lit surface's plane in its direction (the plane comes from how the
  position changes from pixel to pixel). The first alone makes a surface
  that slants away from the light shadow itself, the second alone puts the
  samples behind the wall where a floor ends. With both, a small bias is
  enough (`ShadowBias`, 5 mm, and `ShadowSlopeBias`, 1.5 texels of the map
  for a surface at 45 degrees).
- Map sizes are `ShadowSpotSize` (1024) and `ShadowPointSize` (1024 for each
  of the six faces). The textures are 24-bit depth, bound to two texture
  units past the engine's eight, once with a sampler that compares and
  filters and once with one that returns the depth
  (`iLowLevelGraphics::BeginShadowMap`, `SetShadowMapParams`; the values are
  in the `hpl_State` uniform block).
- The light's amount is multiplied by the result. With linear lighting it
  counts as an amount of light, outside the light power.

- Checked on the 83 reference viewpoints: with hard edges, 81 differ from
  the original shadows in under 1% of their pixels (edges). One of the other
  two is on a map with timed lights. In the other (water cave, view04) a
  lamp lights a wall through its grille that the volumes leave dark, with
  jagged cut-outs next to it: taken for an error of the volumes, not looked
  into further. No GL debug messages. At 2560x1440 the maps cost about 1 ms
  of GPU time a frame in the boat cabin, the office and the refinery (four
  lights with shadows): 0.06 to 0.36 ms to draw them, the rest to read them.
  With the original shadows the reference images are as before.

Fixed after a review of the above:

- A face of a point light's cube map that is not drawn holds the map of
  another light. A blurred sample near the edge of the picture could read
  one: faces are now left out only if they are outside the picture by more
  than the blur and a few texels.
- A spotlight's map is read with the near and far of the projection it was
  drawn with. They were taken from the light, whose reach a fade or a
  flicker changes without the projection following, and every lit point
  then counted as nearer or farther than it was drawn.
- Past the edge of a spotlight's map nothing is in the way; the texels of
  the edge were repeated.
- The fixed blur (`ShadowSoftness`) is no longer limited by `ShadowMaxBlur`,
  which belongs to the blur by distance.
- A map size outside 16 to 8192 in `settings.cfg` gave a bias that was not
  a number: the size is limited before it is used.
- One sample is taken where all samples of the search found something (deep
  in the shadow, as before where none did), and where the blur is narrower
  than half a texel. The samples' places are turned from one to the next,
  not each worked out with a sine and a cosine. Reading the maps takes
  about 0.25 ms less at 2560x1440 (the light passes without the maps: 1.70
  ms before, 1.45 ms after, over 37 viewpoints); the pictures differ less
  than two captures of the same build do.

Two things that the shadow volumes have always done as well, fixed for both:

- An animated mesh that casts a shadow into the picture without being in it
  kept the pose it was last seen in: its vertices are only moved when the
  render list takes it up. What casts shadows from the lights in the picture
  is now brought up to date before the lights are drawn
  (`cRenderer3D::UpdateShadowCasterGraphics`).
- A light that does not move collects what stands still and casts shadows
  once. When its reach changed (a fade, a flicker, a script) the list
  stayed: it is now collected again, and a fade or a flicker that changes
  the reach goes through `SetFarAttenuation`, so a spotlight's projection
  follows too (`iLight3D::UpdateLogic`, `ReachChanged`).

### Shadow panel

- F2 opens a second window, Shadows, next to the reference views: the kind
  of shadows, every setting above, and "Show the shadows alone" (grey where
  a light reaches, dark where its shadow is, for both kinds of shadows).
- It counts what the last frame's shadow maps took: lights, maps, and
  things drawn into them. The time is the "Shadow maps" section of the F3
  overlay, inside "Lights".
- Lights: the lights in view (or all of the map) with their kind, reach and
  how many things cast shadows from them. A light can be picked, to draw it
  alone ("Only this light") or to switch whether it casts shadows, which the
  map or the model decides otherwise (the flashlight casts them).
- A test light: a point light that casts shadows, put where the camera is,
  or carried along with it, with its reach and brightness.

### Player window

- The F2 panel's switches for how the game plays while testing are in a
  window of their own, Player: Fly with its speed, the camera's position,
  and flashlight, flicker, HUD and damage (which were "As in the reference
  images").
- Flying looks around freely: up and down to 89 degrees instead of the
  player's 70, and the player is kept out of the states in which the mouse
  moves a cursor or is held by what the player has hold of (interact mode,
  grabbing, pushing, climbing), since the body those belong to is left
  behind.
- Switching Fly off puts the player where the camera is, facing the same
  way, instead of sending the camera back to where the body was left. So
  does Release at a viewpoint. Before, the camera stayed at the viewpoint
  while the movement keys walked the unseen body.
- Tooltips wrap.

### Ambient occlusion (option)

- **Ambient Occlusion** in Advanced Graphics: Off (the default, the original
  look) or one of five qualities (`AmbientOcclusion` in `settings.cfg`). It
  darkens corners, creases and where things touch: the places that light
  from all around reaches least. The rows of that menu page are 22 instead
  of 24 units apart to make room for it.
- It is worked out from the depth buffer alone
  (`cRendererPostEffects::RenderAmbientOcclusion`,
  `hpl1_PostEffect_AO.fragment`) as ground truth ambient occlusion (Jimenez
  et al. 2016): for a few directions on screen, the depth is searched to both
  sides of a pixel for the horizon, the highest thing in view, and the part
  of the half circle over the surface that the horizons leave open counts,
  weighted by the cosine to the surface's normal. The normals are made from
  the depth buffer too.
- The scene is multiplied by it between the light passes and the rest, so it
  darkens what is lit and nothing else: lamps' lit parts (illumination
  maps), fog, the sky, particles, halos and see-through materials are drawn
  after it. An effect on the finished picture would darken those too.
- The layout of the passes follows Pascal Gilcher's MXAO, the fastest
  implementation of this known to us. Its code is not free to reuse, so
  none of it is here: the shader is written from the papers.
  - The depth is copied as distances (16 bits) and normals into 4x4 tiles,
    each with every fourth pixel (deinterleaved texturing). The search of a
    pixel stays in its tile, where samples far apart on screen are near each
    other in memory. A tile is two textures of its own, and the four tiles
    of a row are written at once from the four pixels they come from
    (`iLowLevelGraphics::SetRenderTargets`), so no pass reads every fourth
    pixel of a texture: that is slow (see "Made faster" below).
  - Every tile has its own turn of the directions and shift of the steps, so
    any 4x4 pixels search 16 different ways. A filter over 4x4 pixels
    averages that pattern away: a guided filter (He et al. 2010) that fits
    the occlusion as a straight line against the distance, which keeps
    edges without weighing each sample.
  - Steps are closer together near the pixel, but each at least about a
    pixel of the tile further than the one before, so no two steps look at
    the same pixel. A sample counts less the further it is from the point,
    down to nothing at the radius, by being drawn back to where the surface
    itself is. Past the picture's edge the pixel at the edge is looked at.
  - At half rate (`AOHalfRate`, off by default) only every other tile is
    drawn in a frame, in turns: every other pixel is a frame old. When the
    view turns fast the shading trails by a frame in those, which showed
    as ghosting in play: it was on by default at first.
- Settings, all in the F2 panel's Ambient occlusion window:
  - `AOSlices`, `AOSteps`: directions per pixel and steps to each side of
    one. The qualities are 2x2, 2x4, 3x6 (High, the default), 4x10, 6x16;
    each has a search program of its own with the two as constants, which
    is faster than other numbers are.
  - `AORadius` (0.6 m): how far from a point things keep light from it.
    `AOScreenRadius`, if not 0, is a share of the picture's height used
    instead, the same on screen at every distance.
  - `AOStrength` (1.6, from play-testing): the power the open share is
    raised to; 1 is the share as computed.
  - `AOLightFade` (0.5): how much less brightly lit parts are darkened. The
    renderer has no picture of the ambient light alone to darken, so direct
    light is darkened too; this holds that back where the picture is bright.
    The blend reads the scene's colour at the pixel it draws over
    (`iLowLevelGraphics::BeginSceneColorRead`, `hpl_SceneColor`: OpenGL 4.5's
    texture barrier); before 4.5 it reads a copy of the scene.
  - `AOFadeDistance` (30 m), `AOFilter` (passes of the filter, 0 to 2; 1).
  - "Show the occlusion alone" draws it as if everything were white, without
    the post effects (`AOShowOnly`, read from `settings.cfg` but not saved).
- With linear lighting the scene is multiplied by the open share; without, by
  that share converted for the screen, which darkens the light by the same
  amount.
- New in the low level: the scene's depth texture stays bound to a texture
  unit of its own (`hpl_SceneDepth`, `hpl1_scene_depth.glsl`; with
  anti-aliasing a copy with one sample a pixel, see "Texture filtering and
  anti-aliasing"), `CreateFloatTexture` makes float textures of one or two
  channels, `SetRenderTargets` draws to up to eight textures at once, and
  the scene's colour is bound to a unit of its own for
  `BeginSceneColorRead`.
- Checked: with the option off all 93 reference frames match the capture
  from before. No GL debug messages with it on, with and without
  anti-aliasing, linear lighting, and at sizes that do not divide by four.
  At 2560x1440 in the office High at half rate costs 2.8 ms of
  GPU time a frame (search 1.6, filter 0.6, the rest 0.6) where that map's
  light passes cost 1.65; Medium costs 2.1 (search 0.9, or 1.6 without half
  rate). The GPU's clock varies between runs: with it higher the same
  passes took two thirds of that. The F3 overlay lists the passes under
  "Ambient occlusion".
- **Made faster** (2026-10-02, after a review of the code and measuring
  pass by pass with `OVERTURE_GPU_LOG`). On an RTX 2060 at 2560x1440 with
  2x multisampling, High, over 37 views in 10 maps, it went from 3.61 to
  2.12 ms of GPU time a frame (42% to 30% of the frame); the picture
  differs only by the two changes to the samples named below (at most 1.2%
  of a frame's pixels by more than 2 of 255). What was found, with the
  time each took off:
  - Reading every fourth pixel of a texture is slow, and worst from the
    copy that a multisampled scene's depth is read from: the pass that
    made the tiles took 0.6 ms for one look-up a pixel, and the search read
    the depth five more times that way for its normal. Now the tiles are
    textures of their own, four written at once with their normals, and
    the depth is read in order, once: 0.28 ms, and the search reads only
    its tile (0.45 ms off in all).
  - The search's time is its arithmetic and its branches, not its
    look-ups (without them it took three quarters as long). Its loop over
    the steps had two ways out, a step that fell on the point itself and
    one that left the picture; the steps are now spread so that none falls
    on the point and the edge is clamped to, which leaves no branch
    (0.3 ms off, and the steps that were thrown away are used). With the
    numbers of directions and steps as constants the compiler writes the
    loops out (0.3 ms off): hence a program for each quality. A tile's
    directions come from the renderer instead of sines and cosines per
    pixel.
  - The horizon's angle is used in one place instead of three: the cosine
    of a difference is written out, which takes an arc tangent and two
    cosines out of every direction, and the arc cosine left is a
    polynomial. The same result within rounding; 0.06 ms off.
  - The blend does the last pass of the filter itself instead of reading
    what a pass before it wrote (0.1 ms off), and reads the scene it draws
    over instead of a copy of it (0.3 ms off with multisampling, where the
    copy needed all samples averaged first; 0.14 without). It is drawn as
    one triangle for that: where the two of a quad meet, a multisampled
    pixel is drawn once for each, and the second would read what the first
    wrote.
  - Simplifying the arithmetic of a sample by hand changed nothing: the
    compiler had done it.
  - Half rate, when asked for, starts from a whole frame after the option
    or ambient occlusion itself was switched on.

### Depth of field, motion blur and grain from shaders (options)

Each is a third setting of an existing option; off and the original are
kept, and the original is what a fresh `settings.cfg` has.

- **Depth of Field: Off / Original / Bokeh** (`DepthOfFieldType` 0/1). The
  original draws every object again to blend the scene with a small blurred
  copy of it. Bokeh works from the depth buffer
  (`hpl1_PostEffect_Bokeh.fragment`,
  `cRendererPostEffects::RenderDepthOfFieldBokeh`): what is out of focus is
  spread over a disc, and what is in focus stays as sharp as it was drawn.
  - What is in focus is still the game's to say: the same near, focus and
    far distances and the same fading in and out as the original.
  - Four passes: the scene at half size with each pixel's blur, a gather
    over rings of samples (Jimenez 2014, laid out as in Unity's
    post-processing stack), a small filter over the gaps between the
    samples, and the result drawn over the scene where it is out of focus.
    What is behind the focus and what is in front of it are gathered apart,
    so a blurred background does not take in sharp things in front of it
    and the blur of near things spreads over what is behind them. The blur
    is made on linear values, so bright points become discs.
  - What is in front of the focus is blurred all through, and what is
    behind it shows through its blurred outline: taken from the samples
    around that are not in front. (At first the sharp picture showed
    through such things, outlines and all.)
  - Between the distance in focus and the ones of full blur the blur
    grows with the square of the way, slowly at first, so there is a zone
    in focus. Growing evenly, as the original's blend does, it left a
    narrow sharp band across a floor that runs through the focus.
  - In front of the focus the blurred picture comes in over the same first
    few pixels of blur as behind it. Switching to it at once, where the
    blur reached a pixel of the half-size picture, drew a line across
    floors and walls near the camera.
  - The game fades the blur in and out evenly; the fade is squared here,
    since a blur of half the width still reads as out of focus and the
    picture seemed slow to come back.
  - `DepthOfFieldRadius` (1.2 % of the picture's height): the blur of what is
    fully out of focus. `DepthOfFieldRings` (3, 1 to 5): rings of samples.
  - "Try it here" in the F2 panel switches the depth of field on at a
    distance of your choosing, with either kind: the game itself only uses
    it when you look closely at something and where a script asks
    (`DepthOfFieldTest`, `DepthOfFieldTestFocus`, `DepthOfFieldTestRange`
    are read from `settings.cfg` but not saved).
- **Motion Blur: Off / Original / Camera** (`MotionBlurType` 0/1). The
  original draws every object again, smeared from where it was the frame
  before. Camera is one pass over the scene
  (`hpl1_PostEffect_MotionCamera.fragment`,
  `cRendererPostEffects::RenderMotionBlurCamera`): where a pixel was the
  frame before follows from its depth and from how the camera moved, and
  the pixel is the average of the scene along that way.
  - Only the camera's own turning and walking blurs. Things that move by
    themselves stay sharp, which the original does blur.
  - `MotionBlurShutter` (1/120 s): the blur covers the way the picture moves
    in that time, so it looks the same at any frame rate. The original's
    amount is a share of the frame, and fades away as the frame rate rises.
  - `MotionBlurMaxLength` (4 % of the height), `MotionBlurSamples` (8, each
    pixel's shifted differently, so few samples show as grain, not as
    copies of the picture).
  - A sample nearer than the pixel counts only as far as its own blur
    reaches the pixel. `MotionBlurNearLimit` (0.6 m): nearer than this
    nothing is blurred, since what the player holds moves with the camera.
  - Pixels that do not move are left as drawn.
- **Noise Filter: Off / Original / Grain / Film Grain** (`NoiseFilterType`
  0/1/2). The original lays tiles of six noise images over the whole
  screen, HUD included. The other two are made by the shader that finishes
  the 3D scene (`hpl1_output.fragment`), on the screen values, under the
  HUD and menus.
  - Grain is noise added to the picture, alike at every brightness.
  - Film Grain takes the picture as made of grains, each a clump of twelve
    crystals that are developed where the picture is brighter than a
    threshold of their own. A pixel shows the share of the crystals over
    it that are developed, which is on average the picture's own value. So
    there is no grain in black or in white and most in between, dark parts
    have faint bright specks and bright parts dark ones, and the same
    crystals come on one by one as the brightness rises over a surface
    (the binomial model of film grain). It does not change the picture's
    brightness. Colour film has crystals for red, green and blue.
  - Settings for both: `GrainIntensity` (0.25), `GrainSize` (1.5: pixels of
    a picture 1080 high; it grows with the picture and is never under a
    pixel), `GrainColor` (0.3; 0 is grey grain), `GrainShadows` and
    `GrainHighlights` (1: the share dark and bright parts get), `GrainRate`
    (24 times a second, as film; 0 is every frame), `GrainDispersion` (0.5,
    film: how much the grains differ in size).
  - prod80's and iMMERSE's film grain shaders for ReShade were looked at
    for what they offer; the code here is written anew (iMMERSE's licence
    allows no reuse).
- All settings are in the F2 panel's Lens and film window.
- New in the low level: `SetSceneColorOnly`, a framebuffer of the scene's
  colour without its depth, so that a pass can read the depth of the scene
  it draws over; `SetFilmGrain`.
- The render list keeps the objects for the original two effects only when
  one of them is on.
- Checked: with the options at their originals all 93 reference frames match
  the capture from before (the menu page's texts aside). No GL debug
  messages with all three on, with and without anti-aliasing and linear
  lighting. GPU time at 1280x720 in the office: Bokeh 0.36 ms, Camera
  motion blur 0.36 ms while turning fast (nothing when still), Film Grain
  0.25 ms, Grain 0.02 ms. Not yet play-tested: all defaults are first
  guesses.

### Texture filtering and anti-aliasing

- **Texture Filter** in Advanced Graphics is one option now: Trilinear, or
  Anisotropic 2x to 16x (as far as the graphics card goes), in place of
  Texture Filter (Bilinear / Trilinear) and Anisotropy. A fresh
  `settings.cfg` has anisotropic 16x; it was bilinear with none. The keys
  are the same (`TextureFilter`, `TextureAnisotropy`), so a file from before
  keeps what it had, and bilinear can still be asked for there.
  - How a texture is read is no longer set on each texture but on sampler
    objects (`cLowLevelGraphicsSDL::GetSampler`), one for each combination
    of wrap modes, with mipmaps and without, bound to the unit along with
    the texture. Changing the filter changes those few samplers
    (`iLowLevelGraphics::SetTextureFilter`) instead of walking every
    material's textures, and it reaches every texture with mipmaps: the
    textures lights shine through were left bilinear before.
  - Setting a texture's filter or wrap mode bound the texture to whatever
    unit was active and left it there, behind the back of `SetTexture`,
    which could then skip binding the texture it thought was still set.
    Wrap modes are only kept by the texture now, and
    `iTexture::SetFilter`/`SetAnisotropyDegree` are gone.
- **Mipmaps.**
  - Textures drawn with the alpha test (fences, gratings, perforated
    metal: the materials with `UseAlpha`) keep what they cut out
    (`iTexture::KeepAlphaCoverage`): the alpha of each mipmap is scaled so
    that the same share of it passes the test as of the full texture.
    Before, thin bars passed less and less the further away they were and
    disappeared, and small holes closed. Far away such a surface keeps its
    pattern now, which also flickers more when nothing smooths it.
  - Cube maps have mipmaps (they were switched off), and are filtered
    across the edges of their faces from the start, not only once a shadow
    map had been made.
  - Texture Quality below High averages the pixels it drops; it kept every
    second or fourth one.
- **Anti-Aliasing** (multisampling, `FSAA`) is changed without a restart,
  and goes to 8x where the graphics card does.
  - The multisampled scene is resolved once (`iLowLevelGraphics::ResolveScene`),
    before the effects that are drawn over the whole of it (depth of field,
    camera motion blur, bloom, image trail). They draw into and copy from
    textures with one sample a pixel; before, each drew into all the
    samples, and each copy resolved the scene again. The original motion
    blur draws what moves once more against the scene's depth, so with it
    the scene stays multisampled until the end, as before.
  - Shaders read the scene's depth from a texture with one sample a pixel
    (`ResolveSceneDepth`, for the ambient occlusion in the middle of the
    scene; `ResolveScene` does it too). They were compiled to read the
    first sample of the multisampled one, which is why changing the option
    took a restart.
  - Alpha-tested surfaces get the samples too: in the multisampled scene
    the test is turned into the share of the pixel's samples that pass
    (alpha to coverage; the alpha is how far past the reference the pixel
    is, over the width of one pixel). The outline a texture cuts out is
    smoothed like the edge of a triangle; before, multisampling did nothing
    for it.
  - `SetMultisamplingActive` (never called) and `CheckMultisampleCaps`
    (empty) are gone.
- **Edge Smoothing (SMAA): Off / Low / Medium / High / Ultra** in Advanced
  Graphics (`SMAA` in `settings.cfg`, off by default). SMAA 1x (Jimenez et
  al. 2012) on the finished scene (`cLowLevelGraphicsSDL::DrawSmaa`,
  `hpl1_SMAA.fragment`): the scene is made into the picture as it is shown
  (tonemapped, screen values), its edges are found, each is followed along
  to tell its shape, and the pixels on it are blended by how much of them
  the true edge would cover. It reaches what multisampling does not (the
  edges of shadows, of textures and of lit areas) and works along with it.
  - The searches, the area and search textures and the blend are the
    reference implementation's (`shaders/SMAA.hlsl`, `vendor/smaa`, MIT).
    What is around them follows iMMERSE's SMAA (Pascal Gilcher), whose own
    code is not free to reuse: the settings are uniforms, where the
    look-ups go is worked out in the fragment shader, edges are found by
    brightness, by colour (the largest difference, or all three together)
    or by depth alone (`SMAAEdges` 0 to 3, 1 by default), and the edges and
    blend weights can be shown. Its compute shader, which gathers the
    pixels with edges and works on them alone, needs OpenGL 4.3; the
    stencil buffer does that here.
  - The depth helps (`SMAADepthHelp`, on): where the distance changes by
    more than 3 % of itself (`SMAADepthThreshold`) the threshold is halved
    (`SMAADepthStrength` 0.5), so outlines are found though the colours on
    both sides are close, as they are in the dark. The reference
    implementation's predication also doubles the threshold everywhere
    else (`SMAADepthScale`, 1 here): that leaves surfaces alone, and the
    edges of shadows with them.
  - The qualities are the reference implementation's: `SMAAThreshold`
    0.15 / 0.1 / 0.1 / 0.05, `SMAASearchSteps` 4 / 8 / 16 / 32,
    `SMAADiagonalSteps` none / none / 8 / 16, `SMAACornerRounding` 25 % for
    the upper two.
  - The output pass is split for it: `hpl1_output.fragment` with
    `ENCODE_ONLY` makes the picture, SMAA puts the blended picture back in
    the scene's texture, and the output pass scales it and adds the grain
    and the dithering as before.
- **Render Scale** is filtered by the output pass itself: above 100% each
  pixel is the average of the scene's pixels it covers, each counted by the
  share covered (the single filtered look-up was that at 200% only, and
  uneven at 150%); below 100% a curve through four pixels each way
  (Catmull-Rom) in place of the straight blend, which is sharper.
- A sixth window in the F2 panel, Anti-aliasing (folded, at the bottom
  right): multisampling, the texture filter (bilinear too), SMAA's settings
  and its views of the edges and the weights (`SMAAShow` 1 / 2, read from
  `settings.cfg` but not saved).
- Checked: with the reference settings (trilinear, no anti-aliasing) 57 of
  93 frames match the capture from before and 34 differ: in what is drawn
  with the alpha test (fences, gratings and perforated metal further away,
  which keep their pattern; the outlines of paper and chains) and where a
  light shines through a textured pattern, which is trilinear now.
  No GL debug messages with SMAA, with 2, 4 and 8 samples, with both
  together and every post effect, linear lighting on and off, render scales
  from 50 to 200%, and switching all three options from the menu; the same
  on Mesa's software renderer. At 2560x1440 in the office SMAA costs
  0.63 ms of GPU time a frame at High and 0.72 at Ultra (F3: "SMAA" under
  "Scene output").

### Improved shading (option, off by default)

The first step of Milestone 5. Settings in the F2 panel's Shading window,
kept in `settings.cfg` (`ShadingImproved` and `Shading*` under Graphics);
the option itself is also on the menu's Lighting page. With it off the
picture is the original's.

- **Highlights.** The original's specular term is one lobe of a fixed width
  (a power of 16) for every material, white whatever the light's colour,
  with the half vector made per vertex. Improved replaces it
  (`hpl_Highlight` in `shaders/hpl1_shading.glsl`) with a microfacet lobe
  (GGX with the Smith visibility term) and Fresnel, per pixel, in the
  light's colour and the spotlight picture's. The game has no gloss maps,
  so a material's specular map (or the alpha of its normal map) gives both
  how much it reflects and how smooth it is: Reflectance is what a white
  map mirrors faced straight on, the two Roughness values are for black
  and white. 31 of the 104 materials that keep the amount in the normal
  map have no alpha there and count as white; the defaults make their
  highlight about as bright as the original's.
- The lobe is widened where the normal map changes from pixel to pixel
  (Tokuyoshi and Kaplanyan 2019), so small bumps far away do not sparkle.
- The highlight is an amount of light. With linear lighting it is added
  as that and can be brighter than white; without, it is converted to a
  screen value and clips at white like everything else.
- **Ambient.** The original's ambient pass is texture x ambient colour, the
  same from every side, so normal maps show only where a light reaches.
  Improved draws the pass with `hpl1_ambient.vertex/.fragment`, which read
  the normal map: more ambient light from above than from below, more on
  what faces the viewer, and for what has specular a sheen at a slant.
- Some faces in the game's meshes have tangents that are not numbers. The
  original shaders happen to draw those with the ambient colour alone; the
  new ones do the same on purpose (`hpl_Normalize`, and the checks in
  `hpl1_ambient.fragment` and `hpl1_light.fragment`).
- The settings travel in the `hpl_Frame` uniform block (`hpl_Shading0/1`).
- `cRenderer3D::RenderZ` leaves texture unit 0 active: the new ambient pass
  sets textures on other units, and the texture environment the renderer
  resets after the pass (which every material without a fragment program
  is drawn with afterwards) is that of the active unit.

### Improved flashlight (option, off by default)

Step 2 of Milestone 5. Flashlight: Original / Improved in the F2 panel's
Shading window, kept in `settings.cfg` (`FlashlightImproved` and
`Flashlight*` under Graphics).

- **A look for each light** (`cLightLook` in `GraphicsTypes.h`,
  `iLight::SetLook`): brightness, tint, falloff by distance, a beam made by
  the shader. The renderer hands it to the light shader with each light
  (`iLowLevelGraphics::SetLightLook`, `hpl_Light0..2` in the `hpl_State`
  block, `shaders/hpl1_light_look.glsl`). The default look is the original
  light exactly, and every light but the flashlight has it.
- **Falloff.** The flashlight's ramp is at full brightness for the first
  two thirds of its 12 m and then drops to nothing. Improved uses one over
  the square of the distance instead: as bright as the original at a
  chosen distance (Even at, 0.4 of the reach), up to 2.5 times brighter
  nearer, dimmer beyond, faded out so that it ends at the same reach.
- **Reach** (6 to 36 m; the original's is 12): the radius the hand model
  keeps for its spotlight is scaled, so the beam, its shadow map and what
  it is drawn on all go as far. Even at is a share of the original 12 m,
  so a longer beam is as bright close up as a shorter one.
- **Beam.** In place of `player_flashlight_gobo.tga` (256 pixels, grey):
  a hotspot in a dimmer field with a soft edge and faint reflector rings,
  computed for each pixel. Hotspot, Field and Width in the panel.
- **Colour.** Warmth tints the light from the original's white towards
  the yellow of a weak bulb.
- **Bounce.** `cPlayerFlashLight::UpdateLook` casts a ray along the beam
  and keeps a point light (`FlashlightBounce`, 4 m, no highlights, no
  shadows) well off what the beam lands on (40% of the way back, 2.5 m at
  most: nearer it is a bright spot in the middle of the beam), as strong as the beam is
  at that distance times Bounce, and eased so that it does not jump at
  edges. It can show through a thin wall.
- The original highlight is scaled by the look's brightness only; the
  improved one (Improved shading) takes the tint too.

### Improved lamps (option, off by default)

Step 3 of Milestone 5. Lamps: Original / Improved in the menu's Lighting
page and the F2 panel's Shading window, kept in `settings.cfg` (`LampsImproved` and `Lamps*` under
Graphics).

- `cLampLooks` (`game/core/LampLooks.cpp`) gives the lights of lamp
  entities, of the thrown flare and of the glow stick and flare in the hand
  a look (`cLightLook`) on every step. The kind of lamp is read from the
  name of the entity's file: paraffin lamps and the flare are flames; the
  iron mine, new storage and desk lamps are bulbs; `modern_mine_flour*`
  and the ceiling lights of the base are fluorescent tubes; then the
  emergency lights and the tunnel lights. Lights of other entities
  (buttons, searchlights, the UV lamp) keep the original look.
- **Flames** waver in brightness (smooth noise, a share of 0.14 for a
  paraffin lamp and 0.3 for the flare) and wander by a few centimetres
  (`cLightLook::mvOffset`, which `iLight3D::GetLightPosition` adds), so
  what they light and the shadows they cast move. Each light has its own
  phase, from its name, and the noise is the same in every run. The
  original's flicker (a slow fade between two colours) still runs
  underneath. Reference captures, and "No flicker" in the F2 panel's
  Player window, hold flames still.
- **Colour.** Each kind has a tint that its lights' own colour is
  multiplied by; Colour (0.5) goes from the original's colours to the full
  tint.
- **Falloff.** The ramp every point light uses stays near full brightness
  for half of the reach. With "Brighter near the lamp" the lamps use the
  flashlight's falloff by distance instead: about the same as the ramp from
  the middle of the reach outwards, brighter close by.
- **Tubes.** `cLightLook::mvTubeAxis` and `mvTubeOffset` say where a long
  light is; the improved highlight is then that of the place on the tube
  the mirrored line of sight passes closest to (Karis 2013), dimmer by as
  much as it is longer (`hpl_TubeDirection`, `hpl_TubeSpread`). The diffuse
  light and the shadows are still those of a point. The tube's place goes
  to the vertex shader for each object (`TubeCentre`, `TubeAxis`).
- Stencil shadow volumes are made from `GetLightPosition` too, so they
  follow a swaying flame.
- **Halos.** A light's halos (the billboards attached to it) are coloured
  by its colour times its look's tint and brightness
  (`iLight3D::OnSetDiffuse`, which `iLight::SetLook` calls now), so a
  paraffin lamp's halo wavers and is tinted with its light.
- **Lights of the maps.** The 56 lights that are in the map files
  themselves, not in a lamp entity, are marked by the map loader
  (`iLight3D::IsPartOfMap`). Nothing says what they stand for, so their
  colours stay; with "Lights of the maps too" (`LampsMapLights`, on) they
  get the falloff by distance if they reach 20 m at most and use the
  common ramp (the sky of the outside map and of the ice cave, the
  propeller's spotlights and the lights with a falloff picture of their
  own keep theirs). The furnace's lights (`furnlight*`) are flames. Lights
  that scripts flash (`*spark*`, `broke1`) are left alone.
- Fixed: the flashlight's settings were written to `settings.cfg` after
  the player object had been deleted; Brightness came out as 0, which is
  no light at all at the next start. They are written before now, and a
  Brightness below what the panel can set is read as 1.

### Halos, light shafts and particles from shaders (options, off by default)

Step 4 of Milestone 5, first part. Halos, Light Shafts and Particles: each
Original / Improved in the menu's Lighting page and in the F2 panel's
Shading window ("Halos, light shafts and particles"), kept in
`settings.cfg` (`HalosImproved`, `ShaftsImproved`, `ParticlesImproved` and
`Halo*`, `Shaft*`, `Particle*` under `Graphics`). With all three off the
reference images are as before.

- A see-through object can draw itself in place of what its material says
  (`iRenderable::GetTransDraw` / `SetupTransDraw`, `cTransDraw`): programs,
  vertex buffer, matrix, depth test and culling of its own.
  `cRenderer3D::RenderTrans` asks each object. The settings are
  `cEffectSettings` (`iLowLevelGraphics::SetEffects`). The scene's depth is
  resolved before the see-through pass when any of the three is on, and
  `hpl1_scene_depth.glsl` has `hpl_SceneDistanceAt()`, which is far away
  where nothing was drawn (the plain conversion can give anything there).
- **Halos** (`hpl1_Halo`, billboards that face the viewer and are added to
  the scene). The glow is drawn from the picture's colour by distance from
  its middle, measured when the picture is first used (32 distances, each
  the average round the ring) and drawn as a smooth curve: no steps or
  blocks from a 256 pixel JPEG. What the picture has besides (the rays of
  the star-shaped halos) is taken from the picture and added ("Rays").
  How much of the lamp is in view is read from the depth buffer in the
  vertex shader: 24 spots on the sides of the lamp's box that face the
  viewer, spread over at least 5 pixels. The original draws that box, or
  the lamp's mesh, with an occlusion query and uses the count a frame or
  more later; here the halo follows in the same frame and fades as an
  edge passes. Lamps whose own mesh is the source are scaled by 0.65,
  which is about what the original's count gives for a lamp in full view.
- **Light shafts** (`hpl1_Shaft`, billboards that turn round an axis, at
  least half a metre long: five kinds in the game, under grates, windows
  and the end door). The picture is spun round the axis into a body of
  lit air; the box round it is drawn and each pixel adds up the air along
  its line of sight in ten steps, between where the line enters the body
  (or the viewer, when inside) and where it leaves it or meets the scene.
  From the side the shaft is about as bright as the sheet was; where it
  meets a wall, the floor or a table it fades out; seen along its length
  it is a disc of light. "Dust" makes the air uneven: streaks that drift
  along the shaft. The window's bars in the boat cabin's shaft become
  rings round the axis and no longer read as bars.
- **Particles** (`hpl1_Particle`, emitters blended by alpha or added).
  They fade out over about their own size in front of what is behind
  them and close to the viewer ("Soft edges"). Those blended by alpha
  (smoke, steam, mist, dust, snow, water) are lit at their corners by the
  six strongest lights that reach the emitter, spotlights by their cone:
  "In the dark" (0.55, plus the map's ambient light) of the original's
  brightness with no light, and never more than the original's. Light is
  not stopped by walls on its way to them. Fire and sparks glow as before.
- Comparison shots (F9) switch the three off for the first picture.
- Found on the way: the reference capture takes a view's frame before the
  original's halo counts have arrived, so halos are missing or dim in some
  reference frames of the original. Nothing was changed for it.

### Lighting page in the options menu

- Options, Graphics has a third page, Lighting, next to Advanced (which is
  full): Shading, Flashlight, Lamps, Halos, Light Shafts and Particles,
  each Original or Improved. Their
  settings stay in the F2 panel. The reference captures include the page
  (`2d-menu_lighting.png`).

### F2 panel: items, and what it switches on its own

- The Player window has a Give row: Flashlight, Glow stick and Flare put
  the item in the inventory (the first two once, flares add up), so their
  keys and the inventory work as in the game on any map.
- The panel no longer switches the flashlight on or hides the HUD by
  itself: "Flashlight on" and "3D view only" start unticked, and going to
  a place leaves the flashlight as it is. The reference captures
  (`tools/refshots.py`) are as before.

### F2 panel: a place is held only while the panel is open

- Fixed: after a map was loaded from the panel (which goes to the map's
  first viewpoint), or a place was picked, the camera stayed at the place
  with the panel closed: the movement keys walked the player's body away
  unseen while the view stood still, until Release was pressed or Fly was
  switched on and off. Closing the panel now puts the player at the place
  (`cRefView::Update` calls `Release()`), and walking and looking go on
  from there. With Fly on the camera flies on from the place as before.

### Comparison shots

- **F9** saves one `overture-compare-<date>-<time>-stacked.png` to the desktop,
  with full-resolution Vanilla above Improved. Improved uses the current
  graphics settings, and labels sit above the screenshots. The original look is captured in memory;
  no separate screenshots are left behind. Windows uses the shell desktop
  folder, including redirected/OneDrive locations. Each look settles for 25
  rendered frames and readback completes before switching settings. The camera
  is held and gameplay input is suppressed during the capture; the F2 panel
  and F3 overlay are omitted. Settings and overlay visibility are restored on
  completion, cancellation, map changes and exit. Moving objects can advance
  between the two captures. No external image tools or Python are needed at
  runtime (`cRefView::StartCompareShots`).

### Fog

- Fogged maps were drawn as a flat fog colour (11 maps: outside, the shafts,
  the lead and iron mines, the water caves, the ice cave). The fog shaders
  took the fog texture's amount from the channel that holds its brightness,
  which is always full, so every surface got all of the fog whatever its
  distance. `hpl1_Fog_Solid`, `hpl1_Fog_Trans` (modulate variants) and
  `hpl1_Water_Fog` now read the alpha the texture swizzle provides. This was
  wrong since the first port; the reference images of those maps change.

### Reference viewpoints

- The reference images of a map are taken from viewpoints chosen by hand,
  kept in `tools/refviews.txt` (map, name, position, yaw, pitch), one frame
  each, named `<map>-<viewpoint>.png`. Maps without any keep the automatic
  start-position and light places.
- The F2 panel sets them: Fly (a free camera; the player's body is switched
  off and stays behind), "Add viewpoint here", "Delete this viewpoint". The
  file is written at once, to the source tree the game was built from
  (`OVERTURE_REFVIEWS_FILE` overrides the path).
- The build copies the shaders on every build, not only after linking: an
  edited shader alone was not copied.
- The physics test map is no longer part of the reference images or of the
  panel's map list.

## Dog AI and animation foundations

- Dogs pursue observed positions after losing sight, follow fresh sound
  locations, and pause/turn during the existing search budget using original
  clips. Hunt thresholds load the authored hunt attributes.
- Clearance checks, obstacle-aware route retries, and short corner lookahead
  reduce blocked-route charges. State transitions stop the old tick; rear-wall
  correction requires a hit and runs separately for each Dog.
- Shared enemy playback uses horizontal gait speed, honours changed options
  when reusing a clip, restores fixed playback speed, and delivers completion
  once. State lookup and perception callbacks preserve the intended state and
  FOV. Turning cannot overshoot; successful backing away is not counted as stuck.
- Door impacts use the authored delay once per tick. Original models,
  textures, clips, jump-attack timing, damage and movement values are retained.
  See [Dog foundations](docs/dog-foundations.md) for balance effects and tests.

## Dog behaviour arena and AI choice

- Options → AI saves mutually exclusive Original/Improved Dog behaviour
  choices. Original restores legacy pursuit/search decisions; both retain
  original assets and shared animation/stability fixes.
- `OVERTURE_MAP=dogtest` opens a generated six-bay arena for patrol, hiding,
  sound investigation, obstacle recovery, doors and attack/get-up. F2 offers
  fresh scenario resets, real sound lures, one-shot position reveals,
  knockdown, perception/damage switches and live diagnostics.
- Arena checks exercise both AI modes against authored navigation, original
  props and sound sources, including save/reload and existing-Dog switching.

## Closed-door Dog attacks

- The arena uses the original mine-shaft door, whose authored health,
  toughness and debris allow the Dog to break it. The previous old-wood
  fixture had no breakable health setup.
- Both Dog AI modes require a clear attack line before lunging, allowing
  door-breaking to start when the player stands immediately behind a door.
- Shared shape attacks block damage, knockback and player-hit feedback
  together, reset skipped-body state and check occlusion before damaging
  obstacles. The strike that breaks a door cannot also hit the player behind it.
- Regressions exercise natural door-breaking in both modes, nearby and distant
  players, solid walls and unobstructed bites using the original assets.

### Player feedback and effects lab

- Improved player feedback runs in the final scene shader, after tone mapping
  and SMAA, below the HUD. Night adaptation eases in with a small FOV zoom,
  blue shadow lift and faint vignette, keeping the aspect ratio correct.
  Blood hits use soft splashes and droplets, randomized once per impact,
  alongside a red vignette. Low health has a stronger heartbeat vignette
  and gently pulsing vision doubling. The second outline stays visible between
  heartbeats, grows with low-health severity and eases away after healing.
  The lab can preview it independently of the red vignette. Ice hits use noisy frost. Panic pulses a small FOV zoom,
  desaturation, dimming and blur. White flashes spread highlights before
  reaching white; death fades smoothly to black.
- The original treatment remains available in F2's Player effects controls,
  alongside adaptation strength/time, injury, panic and whiteout glow. Settings
  persist under `Graphics.Player*`. Improved is on by default. Hiding and
  enemy panic thresholds, damage, audio and the physical death fall remain
  driven by their existing controllers.
- Player image trails have their own controls. Improved save points use warm
  haze and glow instead of the strong zoom and image trail; underwater adds
  a water-colored tint and light blur. Their original rendering remains
  available with the original treatment. The flashlight beam/shader is unchanged. Effect amounts are
  independent and cleared each frame; reference captures that hide the HUD
  also omit player feedback. With all amounts zero the scene is unchanged.
  Scripted fear now fades out as intended, and zero-duration flashes finish
  without division by zero or leaving a flash amount behind.
  Resetting an interrupted save also clears its image trail and overlay.
- `OVERTURE_MAP=effectslab:link01 ./run.sh` opens a compact generated room and
  its F2 panel. A dark alcove, lamps/props, save artefact and pool provide live
  triggers. Safe previews include combined feedback and a tour; live controls
  also cover shake, ear ringing, black fades, focus blur, grain and rocking.
- `tools/refshots.py effects /tmp/effects` captures both treatments, opaque
  endpoints and the panel, checks pixels and exercises real effect owners.

## Windows build

- Builds with Visual Studio (MSVC, x64) and vcpkg: `vcpkg.json` lists the
  dependencies, the `windows` CMake preset uses vcpkg's toolchain, and
  `platform\windows\build.cmd` sets up the compiler and builds into
  `build-windows\`. AngelScript's native calls on x64 need its MASM file,
  now part of the build. The test programs need GCC/Clang and are not built.
- One `main()` on all platforms: on Windows a GUI program (no console) that
  starts there, with a manifest that makes UTF-8 the process code page and
  asks for per-monitor DPI awareness. The C locale is UTF-8 too, so file names
  and the multibyte/wide conversions behave as on Linux.
- The original's Windows-only code was out of date: folder checks compared
  attributes exactly (a folder with any other attribute did not exist),
  renaming always failed, the directory search had a fixed 256-character
  buffer and never closed its handle, and leftover `#pragma comment(lib)`
  lines asked for SDL 1.2 and other libraries by their old names.
- Sound uses OpenAL Soft's default device on Windows too, with EFX when
  environmental audio is enabled. The original asked for the "Generic Hardware/Software" devices of
  Creative's old OpenAL, which no longer exist.
- Settings, saves and `hpl.log` are in
  `Documents\Penumbra Overture\OvertureRemade\`, beside the original game's
  `Episode1` folder, which is not touched.
- `platform\windows\run.cmd` runs the game against the Steam copy (whose
  data is in `Penumbra Overture\redist`), `smoke.cmd` runs the smoke test, and
  `platform/windows/sync.sh` mirrors the source from WSL to a Windows drive.
- `OVERTURE_HIDDEN=1` creates the window hidden and never fullscreen, for
  automated runs where there is no headless compositor like gamescope. The
  Windows smoke test uses it with `ALSOFT_DRIVERS=null`: nothing is shown or
  heard.

## Physics bodies that turn to NaN

- Newton 2 can blow a body up to NaN. Entities placed with a scale give
  Newton scaled matrices, which it does not support, and some that start out
  overlapping do it at once: two of the rocks in the pile in
  `level01_06_workshop` vanished as the map loaded.
- A NaN bounding box also broke Newton's broadphase. Its cells keep bodies
  sorted, and a ray stops at the first one past its end; NaN compares as
  neither, so every ray stopped there. Footsteps (and anything else using a
  ray down) found no floor on the bodies sorted after it: the wooden floors of
  the workshop's corridors were silent. After loading a save the order was
  different, which hid it.
- A body whose pose or velocity is not finite after a step is now put back to
  its last finite pose, at rest, before Newton or the game sees it
  (`dgBody::RestoreIfNotFinite`).

## Graphics presets

- Options → Graphics starts with High, Medium or Low, applied at once.
  It shows Custom when the options have been changed by hand.
  - High: bloom, depth of field, ambient occlusion High, SMAA High,
    shadow maps and 16x anisotropic filtering.
  - Medium: ambient occlusion Low, SMAA Medium, 8x.
  - Low: no ambient occlusion or depth of field, SMAA Low, 4x.
  - All: motion blur off and render scale 100%. Resolution, gamma, field of
    view, the noise filter, VSync and the frame limit are not part of them.
- First run (no settings file yet) uses High at the desktop resolution.
- Presets follow Milestone 6's single improved renderer. The superseded
  Original preset and F9 Vanilla comparison are removed; their pre-merge
  implementations remain in commit `22bbc47`. Windows hidden-window support,
  frame capture callbacks and the tested comparison image compositor are
  retained in the new SDL window component. Windows Overture settings retain
  their separate `Documents\Penumbra Overture\OvertureRemade\` directory.

## Milestone 6: the rest of the 2007 renderer

### Dead code removed

What the game never reaches, with the reference images unchanged (original
look and every improved option on):

- the 2D engine of HPL1, made for 2D games and never used by Penumbra:
  `cWorld2D` and `cScene::LoadMap2D`, tile maps and tile sets, 2D lights and
  their stencil shadows, 2D cameras, 2D bodies and colliders, image
  entities, 2D particles, background images and `cRenderer2D`. Materials
  no longer take the 2D renderer, and the 2D entity and area loaders and
  the tile set and image entity managers are gone from `cResources`. The
  tag `hpl1-2d-engine` marks the last commit that has it
- the `Diffuse2D` material (`diff2d`), which no file of the three games
  names
- low-level graphics functions nothing called: `DrawRect`, `DrawTri`, the
  triangle batch and its extra texture coordinate arrays, two
  `AddVertexToBatch` overloads, `TranslateMatrix`, `ScaleMatrix`,
  `GetClipPlane`, `GetPixelFormat`, `SetBlendFuncSeparate`, the 2D line,
  rectangle and circle drawing, and the commented-out stencil functions
- about 700 lines of commented-out code in the renderer, materials,
  particles, meshes, lights and the GL implementation

`tools/refshots.py --windows` runs the Windows build from WSL, hidden and
silent, in place of the gamescope session.

### Post effects without screen copies

- `CopyContextToTexure` is gone. Bloom, depth of field, motion blur,
  ambient occlusion, the image trail, refraction and the dream filter draw
  the scene into a texture of their own (`DrawSceneCopy`, shader
  `hpl1_scene_copy`), pixel for pixel from the scene texture. The textures
  keep their format: 8 bits without Linear Lighting, so values clamp at 1
  and round as the copies did, and float with it
- rectangle textures are gone (`eTextureTarget_Rect`, `sampler2DRect`).
  Shaders read exact pixels with `texelFetch`, and filtered reads sample
  the same places in 0..1 coordinates
- the depth of field's sky pass (`PostEffect_DoF` with `SKY`) and the image
  trail (`hpl1_PostEffect_ImageTrail`) have shaders of their own in place
  of texture environments. `DrawQuadMultiTex` is gone, and the blur has
  one program for both of its passes

Checked on Windows against the build before it, 95 reference images each:
original look, every improved option, original and camera motion blur, and
Render Scale 1.5 with 4× FSAA are the same apart from the known noise
(the keypad, and a floor edge in icecave-view04 that also changes between
runs of the old build). The 41 images of the effects lab, which run the
image trail, are the same.

### One default shader, no texture environments

- `cFixedFunctionShaders` is gone. What is drawn without a program of its
  own (the HUD, fonts, menus, particles, billboards, beams, transparent
  materials without fog, debug lines, the sky box) uses one shader,
  `hpl1_Default`: the vertex colour, times the texture on unit 0 (2D, or a
  cube map with `CUBE`). Every such draw was that already
- gone from `iLowLevelGraphics`: `SetTextureEnv`,
  `SetTextureConstantColor`, `SetActiveTextureUnit`, `SetColor`, the clip
  planes (nothing used them) and the texture environment enums. The alpha
  test is one cut-off, `SetAlphaCutoff` (0 for off), in place of the eight
  compare functions of `glAlphaFunc`; every caller asked for "at least 0.6"
- gone: `iGLStateProgram` and the texture environment states of the
  environment map, Modulative and ModulativeX2. None was read: the
  environment map draws with its vertex program's own fragment shader,
  and the Modulative ones only stood in for a fog program that always loads
- the item flash in `MapHandler` draws materials without programs with
  `Diffuse_Color` and `Ambient_Color`, as the others already were
- the light shader normalizes the light vector with `normalize()` instead
  of the normalization cube map, and computes the spotlight's cut-off
  behind the light instead of reading `core_spot_negative_reject`.
  `cTextureManager::CreateAttenuation` (a 3D texture nothing read) is gone

Checked on Windows against step 2, 95 reference images each. All of the
change is the normalized light vector: with the cube map kept, the
original look and every improved option are the same (known noise apart).
With `normalize()`, about 30 frames of each set differ, mostly by 1 to 3
levels on lit surfaces; a few hundred pixels a frame at most go past 4
levels, at the edges of lights, and single pixels reach 30. The effects
lab shows the same.

From here the original look is checked to the eye, not exactly: a few
levels are fine.

### One look

The improved look is the only one. The original 2007 look was the
reference it was tuned against; the tag `original-look` marks the last
commit with it, and its reference images are kept in the Windows kit
(`captures/original-look`).

- the improved settings are the defaults, and `refshots.py capture` takes
  the improved look without extra settings, with shadows, ambient
  occlusion, SMAA and 4x multisampling on. `Graphics.Shadows=2` is shadows
  off: the improved captures until now had it, so they never showed the
  shadow maps
- linear lighting only: the Linear Lighting option, the branches of the
  light and post-effect shaders for a scene of screen values, the 8-bit
  screen copies and `SetLinearSource` are gone. The dream filter (Black
  Plague) draws its copy of the scene with a shader of its own
  (`hpl1_DreamFilter`). `HPL_LINEAR_COLOR` stays, to mark the shaders that
  write linear values, and so does the alpha matching: without it smoke,
  snow and fog cover more (31 frames change, outside by 46 levels on
  average)
- shadow maps only: the stencil shadow volumes are gone, with
  `ShadowType`, the `ShadowExtrude` programs, the triangle normals and
  edges of every submesh (recomputed each frame for skinned meshes), the
  doubled positions in vertex buffers and the stencil functions of
  `iLowLevelGraphics`. Shadows is On, Only Static or Off
- the bloom chain, bokeh depth of field and camera motion blur only: the
  original bloom, depth of field and motion blur are gone, with their
  types, the blur passes, the objects drawn again for them and the
  previous matrices of renderables and cameras. Bloom, Motion Blur and
  Depth of Field are On or Off
- the improved shading, flashlight, lamps, halos, shafts, particles and
  player effects only: their `...Improved` settings and the original paths
  are gone, with the occlusion queries that faded halos, the image trail,
  and the overlay pictures, zooms and hiding light of the original player
  effects. The Lighting page of the menu keeps Refractions; the settings
  are in the F2 panel
- the F9 comparison shots, which switched to the original look, are gone

Checked on Windows area by area, each against the capture before it, 95
reference images each:

- defaults against the improved look of step 3: the same (the keypad
  apart)
- linear lighting only: every 3D frame the same; the menu pages lose their
  rows. Motion blur and Render Scale 1.5 with 4x FSAA the same, apart from
  the known noise; the effects lab the same apart from original-combined,
  which changes as much between two runs of one build
- shadow maps only, against a new capture with shadows on: the same apart
  from the menu and the known noise (icecave-view01/04, 101 and 836 px)
- the post effects: the same, also with motion blur (93 of 95), Render
  Scale (92 of 95) and in the effects lab (41 of 41)
- shading, flashlight, lamps, halos, shafts and particles: the same apart
  from the Lighting menu page; the halos without occlusion queries change
  no frame
- player effects: the same. The effects lab has 29 frames; all checks
  pass, and panic, double vision and combined show the FOV pulse at a
  slightly different point (edges only, up to 76 levels at a few pixels)

### No matrix stack

Draws supply model, view and projection together in `cDrawMatrices`, through
`SetDrawMatrices`. The matrix stacks, push/pop/identity methods, `eMatrix`
and the unused texture matrix are gone.

- world geometry, lights, transparent materials, item outlines and debug
  geometry keep model and camera view separate; shadow casters receive
  their light's view and projection explicitly
- the sky box uses its scale and a view without camera translation
- post effects, the intro and texture overlays use identity model/view
  with an explicit orthographic projection; temporary passes save and
  restore the complete transform, including projection
- 3D GUI sets supply their own camera projection; 2D sets explicitly
  select a screen-centred or local orthographic projection. The projection
  helpers return matrices instead of changing projection state
- shaders take matrix values only: the Cg-style state-matrix overload of
  `SetMatrixf`, `eGpuProgramMatrix` and `eGpuProgramMatrixOp` are gone.
  The combined matrix is still computed as projection × (view × model)

Checked with Linux and Windows builds and the engine regressions. Windows
reference comparisons against step 4:

- defaults: all 95 frames match within the existing tolerance
- Render Scale 1.5 with 4× FSAA: all 31 frames match (office, water caves,
  ice cave and outside, plus the 2D screens)
- effects lab: all 29 frames match; the pixel and live-transition checks pass
- motion blur: 94 of 95 match. The keypad's inventory borders draw over
  the panel in one run: both use the same Z and the batcher sorts ties by
  texture address. A repeat of the same new build changes those borders
  back to the baseline, confirming the existing instability. That repeat
  also shows the known ice-cave noise (104 and 837 pixels over tolerance
  in view01/04)

### Materials and linked programs

- Each program links its vertex and fragment shaders together and draws
  with `glUseProgram`. Materials, refraction, transparent objects, post
  effects and the default/UI shader select one complete program. The Cg
  stage enum, independent bindings, companion stages and separable
  pipelines are gone; separate shader objects are no longer required.
- `CreateProgram` takes a GLSL base name, explicit defines and an optional
  different vertex shader name. Light, fog, ambient, bloom, AO and bokeh
  variants use those requests directly. Resource keys distinguish both
  stages and case-sensitive defines; `CgNameToShaderBase` and its variant
  translation table are gone.
- Material, object and post-effect values use `hpl_Material`, a std140
  block at binding 2. Typed `eProgramParam` slots replace name-based
  uploads, including array elements; setters update CPU values and a
  dirty block is uploaded once before drawing. Each linked program owns
  its block. Frame and render-state blocks stay at bindings 0 and 1;
  sampler locations and bindings are resolved once when linking.
- Hot reload watches both stages and their includes for every program,
  including internal fullscreen passes. A replacement becomes active
  only after both stages link and its material layout is checked. Failed
  edits retain the old program; material values survive successful and
  failed reloads.
- Contiguous draw queues replace the twelve-level render-state tree,
  its node/state pools and recursive traversal. Each draw has one
  lexicographic key, preserving sector/pass and state priorities. A
  change reapplies the affected state and everything after it, including
  sector ambient setup and per-object/light matrices. Transparent objects
  keep their back-to-front order; sprite batching is unchanged.
- Engine regressions cover isolated parameter blocks, dirty tracking,
  column-major matrices, array bounds and draw-queue state transitions,
  including equal-key draws and material setup changes.

Checked with the Linux and Windows builds and the engine regressions.
The CPU and GLSL material layouts agree on all 68 fields (2,160 bytes),
and linked programs also check the driver's block size. Windows reference
comparisons use the existing tolerances:

- defaults: 94 of 95 frames match the fresh step 5 baseline; the keypad's
  equal-Z inventory borders account for the difference
- motion blur: 93 of 95 match step 5; icecave-view01/04 differ by 101/836
  pixels, the previously reproduced noise
- Render Scale 1.5 with 4× FSAA: 29 of 31 match step 5 (office, water
  caves, ice cave, outside and UI); icecave-view01/04 differ by 113/727
  pixels
- effects lab: all 29 frames match step 5; the pixel and live-transition
  checks pass
- shader reload: all 95 frames match the normal step 6 capture. Valid
  edits, invalid vertex/fragment/include edits and recovery were exercised
  on the defaults, bloom and internal output passes, with no invalid GL
  operations. All temporary build-shader edits were restored

Windows validation used the existing test copy; focused patches preserved
its platform and capture additions.

### One sprite batcher for 2D

- `cSpriteBatch` builds textured triangles for the HUD, GUI, bitmap glyphs,
  fades, loading screens and story overlays. Each batch carries a texture,
  blend enable and source/destination factors. Only adjacent compatible
  sprites merge, so blending never changes the requested order. Replace,
  alpha, additive, source-alpha additive, modulative and smoke blends keep
  their existing factors and shader colour handling.
- Sprites carry position (including Z), colour and texture coordinates.
  The existing HUD size/rotation/translation geometry, atlas coordinates
  and GUI size/colour multiplication are retained. Batches draw using the
  caller's matrices, depth and culling; GUI clip changes flush first.
  Screen-space and 3D GUI sets use the same path.
- Equal-Z HUD and GUI sprites now retain submission order. The old texture,
  material and clip-region address tie-breakers could change which
  translucent sprite covered another between identical runs. This is an
  intentional ordering correction, including the keypad's inventory
  borders, rather than a tolerance change.
- `AddVertexToBatch` and its variants, index/quad batch calls, `DrawQuad`
  overloads and the fixed 20,000-entry arrays are removed. The batcher uses
  growing vectors; sprite material callbacks/type keys and GUI blend
  callbacks become declarative blend descriptions. Unsupported world
  materials still do not enter the HUD sprite path.
- World/debug faces, refraction masks, fullscreen post effects and the
  screen-copy overlay submit explicit triangles through `DrawTriangles`,
  keeping their own programs and state. General mesh buffers, textures,
  fullscreen-pass architecture and backend splitting stay outside step 7.
- Engine regressions check triangle winding and vertex data, adjacent
  texture/blend boundaries, more than the old array capacity, GUI blend
  factors and stable HUD/GUI depth ties.

The Linux build and engine regressions pass, including the new sprite
checks. The Windows MSVC build passes; this test copy explicitly disables
CTest targets under MSVC, so its validation uses the game captures.
Focused patches preserve its Windows and capture additions.

At unchanged comparison tolerances:

- a fresh step 6 baseline (`/tmp/step7-before`) and the batcher-only stage
  (`/tmp/step7-batcher`), before changing ordering, match in all 95 frames
- the final default capture (`/tmp/step7-after`) matches 94 of 95 baseline
  frames. Only `2d-keypad.png` changes (7,225 pixels): the inventory borders
  now stay beneath the keypad panel. Inspection of the before/after/diff
  images confirms the correction is confined to those borders
- motion blur (`/tmp/step7-after-motion`) matches 92 of 95 step 6 frames:
  the same keypad correction (7,225 pixels) and the previously reproduced
  icecave-view01/04 variations (103/839 pixels)
- Render Scale 1.5 with 4× FSAA (`/tmp/step7-after-scale`) matches 29 of
  31 step 6 frames with the same office/water-cave/ice-cave/outside filter.
  Only icecave-view01/04 vary (114/727 pixels); all UI frames match
- the effects lab (`/tmp/step7-after-effects`) matches all 29 step 6
  frames. Pixel checks and live transitions pass. OpenGL debug output is
  enabled, with no GL errors or material-layout mismatches (the driver
  reports only synchronized pixel-readback performance warnings)

The fresh unchanged step 6 run itself reproduces the ice-cave variation:
icecave-view01/04 differ from `/tmp/step6-after` by 101/837 pixels before
any step 7 code is applied.

### Step 8: vertex buffers and textures

- Every mesh buffer uploads enabled fields into one interleaved VBO, with
  one element buffer for indices and explicit offsets/stride in its VAO.
  Dynamic updates gather all enabled fields, retaining UVs, colours and
  four-component tangents when only positions/normals change. Packed CPU
  attribute arrays remain the editing contract: skinning, tangent creation,
  bounds, Newton collision geometry and mesh caches never read GPU strides.
  Copies own their editing data and preserve tangent handedness.
- UVs and tangents are `eVertexFlag_TexCoord`/`eVertexFlag_Tangent` and
  `hpl_TexCoord`/`hpl_Tangent`. The unused colour and coordinate slots are
  removed. MSH import expands xyz positions to xyzw; export writes xyz and
  uses each attribute's own element count, correcting its submesh-index
  stride bug. Collada caches retain their packed positions and tangents.
- The skybox creates triangle indices with the same diagonal and winding
  as before. The quad draw type, second quad element buffer and draw-time
  conversion are removed. Collada polylist/polygon corners are triangulated
  before vertex splitting, preserving their position/normal/UV tuples;
  geometry cache version 2 rebuilds older caches through that loader.
  Debug lines keep their line primitive. Billboards, beams and particles
  retain their existing CPU update paths and triangle topology for step 10.
- Resource textures, float targets, scene/resolve/output textures,
  multisampled colour/depth and shadow maps allocate immutable storage.
  Texels arrive through subimage uploads, framebuffer rendering or copies.
  Repeated atlas uploads reuse storage; changed dimensions/formats replace
  the GL object and refresh every cached texture-unit binding. Failed
  allocation retains the previous storage. Render-target binding reattaches a resized resource even
  when its pointer is unchanged. Mip counts, quality averaging,
  luminance/alpha swizzles and sampler filtering/wrapping are retained.
- Animated textures allocate one 2D array, with one layer per frame. The
  existing fractional time, loop/oscillate and manual stepping controls
  select a layer at draw time. Alpha coverage is retained independently
  per layer. Frame dimensions/formats and the driver layer limit are
  checked before upload.
- Linked programs cache only the sampler layouts they use: an animated
  diffuse, normal, specular or spotlight map reads a sampler2DArray on the
  same unit that a static sampler2D uses. `hpl1_texture.glsl` supplies the
  sampling/LOD/fetch/size helpers. Layer uniforms resolve at link time;
  material values share the program's existing UBO. Reload rebuilds every
  used layout as one transaction and retains the full working set if any
  stage, include or layout fails.
- The context chooser still accepts GL 3.3 with both immutable-storage ARB
  extensions, loading their entry points explicitly on older contexts.
  GL 4.3 supplies both in core. Drivers lacking either receive a startup
  error. Backend splitting remains step 9.
- Engine regressions cover interleaved fields after partial updates,
  pointer stability, independent mesh copies, tangent handedness, polygon
  winding/UVs and cache invalidation. An optional Linux EGL regression
  runs actual GL storage and sampling without a window or game data:
  mip counts, atlas updates, array layer contents/timing, quality averaging,
  per-layer alpha coverage, filtering/wrapping, resized FBO attachments,
  mixed material sampler layouts and successful/failed include reloads.
  CMake enables it when EGL headers/library are available; CTest reports a
  skip if a surfaceless GL 4.3 context cannot be created.

`cmake --build --preset default -j 8` and
`ctest --test-dir build --output-on-failure` pass on Linux: 2/2 suites,
including `renderer_storage` on llvmpipe (LLVM 21.1.8). Linux headless game
captures still require the unavailable gamescope; the EGL suite does not.
The separate Windows test copy passes
`cmd.exe /c platform\windows\build.cmd -j 6`. Its MSVC configuration disables
CTest targets, so Windows validation is the build and game captures.
Focused patches preserve its platform and capture additions.

Windows reference comparisons against the step 7 captures, using the
existing tolerance 2 / maximum 64 changed pixels, on an RTX 2060 / GL 4.6:

- the vertex-only stage (`/tmp/step8-vertices`) matches all 95 frames
- final defaults (`/tmp/step8-after`) match 93/95; only icecave-view01/04
  vary (106/837 pixels)
- motion blur (`/tmp/step8-after-motion`) matches all 95 frames. GL debug
  output is enabled across every map and UI screen, with no GL errors or
  shader/material-layout errors; only synchronized pixel-readback
  performance warnings appear
- Render Scale 1.5 with 4× FSAA (`/tmp/step8-after-scale`), filtered to
  office/water_cave/icecave/outside as before, matches 29/31 frames; only
  icecave-view01/04 vary (114/727 pixels)
- the effects lab (`/tmp/step8-after-effects`) matches all 29 frames;
  neutral/visible/opaque pixel checks and live transitions pass, with GL
  debug output enabled and no GL errors (only the same readback warnings)

All 2D frames match step 7, including inventory borders remaining beneath
the keypad. The ice-cave variation was reproduced before this milestone
step (see step 7 above).

### Step 9: SDL graphics backend ownership

`cLowLevelGraphicsSDL` keeps the `iLowLevelGraphics` contract and coordinates
initialization, frame presentation and resource factories. Its former private
GL/SDL resources now belong to six components:

- `cSDLGraphicsWindow`: SDL window/context lifetime, display selection,
  cursor/V-Sync controls and frame capture.
- `cGLGraphicsState`: default linked programs, frame/state UBOs, matrices,
  shader-visible state, current attributes and texture/sampler/VAO caches.
- `cGLRenderTargets`: scene, resolve, output and resource-target framebuffers,
  render dimensions, multisample/depth resolves and target transitions.
- `cGLFinalPasses`: scene-copy, output, gamma and SMAA programs/resources,
  grain/player feedback and temporary fullscreen GL state restoration.
- `cGLShadowTargets`: spotlight/cube depth targets, comparison/raw samplers
  on units 8/9, shadow rendering and restoration of the prior draw target.
- `cGLStreamDrawing`: streamed triangles/sprites, debug lines/boxes/spheres,
  and streamed indices used by mesh draws.

Components use explicit dependencies and destination facts, without a pointer
back into the facade's private state. Constructors allocate no GL resources;
GL components also initialize independently under an external EGL context.
Cleanup releases programs, framebuffer attachments, buffers, textures and
samplers while the context is still current, before SDL destroys it. Reset is
idempotent, including partial or uninitialized teardown. A scene generation
counter invalidates render-sized SMAA textures after scale/FSAA changes;
lookup textures remain shared across sizes.

The frame order remains scene → resolve/effects → SMAA/output → HUD → gamma
→ debug overlay/capture → window swap. Texture units 0–7, shadow units 8/9,
scene depth/colour units 10/11, immutable storage, animation sampler layouts,
equal-depth sprite order and CPU billboard/beam/particle updates are retained.
The latter geometry rewrite remains step 10.

Linux engine regressions now construct and destroy the backend without loading
GL. The EGL suite additionally exercises scene colour/depth resolve, colour-only
scene targets, scene copies, resource-target resizing, multiple-to-single
attachments, spotlight/cube targets and viewport/framebuffer restoration,
alpha-to-coverage transitions, fullscreen program/material/sampler/capability
restoration, repeated scale/FSAA/SMAA rebuilding, streamed draws after fullscreen
passes and deletion of the owned GL objects.

`cmake --build --preset default -j 8` and
`ctest --test-dir build --output-on-failure` pass: 2/2 Linux suites, including
actual EGL execution on Mesa llvmpipe (LLVM 21.1.8). Linux game captures still
need the unavailable gamescope. The Windows test copy passes
`cmd.exe /c platform\windows\build.cmd -j 6`; its MSVC kit disables CTest.
Focused backend patches retain its hidden-window, comparison-image and frame
capture additions.

Windows comparisons against step 8 on the RTX 2060 / GL 4.6, with the existing
tolerance 2 / maximum 64 changed pixels:

- defaults (`/tmp/step9-after`): 93/95 match; only the known icecave-view01/04
  variation differs (106/837 pixels). All 2D frames match.
- motion blur (`/tmp/step9-after-motion`): 95/95 match.
- Render Scale 1.5 with 4× FSAA (`/tmp/step9-after-scale`), filtered to
  office/water_cave/icecave/outside: 31/31 match.
- effects lab (`/tmp/step9-after-effects`): 29/29 match; neutral/visible/opaque
  pixel checks and live effect transitions pass.

Motion/effects runs have GL debug enabled, without GL or shader/material-layout
errors; only the existing synchronized pixel-readback performance warnings
appear. Logs are `/tmp/step9-final-{linux-build,linux-ctest,windows-build}.log`
and `/tmp/step9-{default,motion,scale,effects}-{capture,compare}.log`.

### Step 10: billboards, beams and particles

Effect sheets use `cQuadInstance` records through `iVertexBuffer`, retaining
`iLowLevelGraphics` and the existing render queues. Each 88-byte record holds
a centre, two half-extent vectors, UV bounds and endpoint colours. Builders
fill retained instance storage directly; they no longer allocate or update
four position/colour/UV vertices per particle. The particle vertex payload
falls from 176 to 88 bytes per live particle, and the per-emitter maximum-size
index/vertex arrays are gone. CPU simulation, spawning, warm-up, lifetime,
random calls, texture-frame selection and save/gameplay code are unchanged.

`cGLStreamDrawing` owns one static six-index triangle buffer shared by all
effect buffers. Four constant shader corners plus `glDrawElementsInstanced`
expand the sheets, retaining indexed vertex reuse for the halo visibility
and particle-light calculations. Each object/emitter stays at its existing
place in the transparent queue; particles retain their array order. The
vertex-buffer adapter owns its instance VBO/VAO, uploads changed records on
bind, skips empty draws and counts triangles for the profiler. Debug line
drawing and on-demand vertex inspection remain available. No expanded CPU
arrays are retained for effect rendering.

The shared vertex input include (`hpl1_quad.glsl`) supplies the expanded
position, colour, UV, normal and tangent to every linked program, including
default, material, depth, fog, refraction, halo and soft-particle programs.
Ordinary meshes, 2D streams and fullscreen draws retain their named input
locations. Instancing uses divisor attributes available in GL 3.3; no new
version or extension requirement is introduced. The instance-kind attribute
returns to zero after drawing so subsequent ordinary VAOs keep their input
contract. Programs still use material binding 2 and frame/state bindings 0/1;
all sampler units, array layers and atomic reload behavior are retained.
Failed `PrepareDraw()` calls skip instance rendering as they do mesh draws.

Billboard point/axis facing, forward offsets and UV orientation are unchanged.
Beams retain endpoint alignment, endpoint colours/alpha multiplication and
tiled/non-tiled height (including the original negative terminal UV). The
alpha-multiplication flag still takes effect separately when each endpoint
colour setter runs; geometry updates do not recolour the other endpoint.
Particles retain fixed/dynamic point, spin, line and axis drawing, both
coordinate systems, subdivision order and stationary-line fallback. CPU work
now prepares a centre and basis rather than corners. Bounds/culling remain
with their existing owners. These effects still do not cast shadows; opaque
material depth/fog draws use the same expansion. Halos keep their visibility
shader and shafts keep their cached volume box. Post effects continue to read
the scene depth/colour with the existing pass order and soft-particle depth
behavior.

The parsed `PEType=Beam` particle mode had no beam-specific geometry/render
implementation before this step and remains non-drawing. Its existing
particle simulation is unchanged; standalone `cBeam` is fully converted.
Particle interpolation, simulation on the GPU and combining transparent
objects into reordered batches are outside this step.

Regression coverage compares production builders with the previous corner
formulas for every particle orientation, spin and coordinate system, UV
subdivisions, billboard alignment/offsets and beam endpoints/tiling/colour.
The EGL suite compares instanced and expanded reference output across blend
modes and animation layers, checks actual soft-particle alpha/additive depth
fades, element limits, empty submissions, copies, shared topology ownership,
resource teardown, ordinary draws after instances, failed sampler layouts and
valid/invalid geometry-include reloads. It also links fog/halo/shaft programs.

`cmake --build --preset default -j 8` and
`ctest --test-dir build --output-on-failure` pass (2/2 suites), including real
EGL execution on Mesa llvmpipe (LLVM 21.1.8). The storage suite also passes
with Mesa restricted to GL 3.3 plus the existing storage extensions, and to
GL 4.3. Its EGL setup now loads the ARB storage entry points just as the SDL
window owner does. No context is needed for backend construction; allocated
instance and shared topology resources are released before context teardown.
The Windows MSVC build passes; its kit disables CTest targets, so Linux
regressions are reported separately. Focused patches retain the Windows
copy's platform and capture additions.

Windows comparisons against the corresponding step 9 captures, on the RTX
2060 / GL 4.6, retain refshots tolerance 2 / maximum 64 changed pixels:

- indexed stage defaults (`/tmp/step10-after`): 95/95 match; all UI frames
  match. The subsequent beam setter preservation is covered by regressions
  and the final captures below.
- final motion blur (`/tmp/step10-after-motion`): 95/95 match.
- final Render Scale 1.5 with 4× FSAA (`/tmp/step10-after-scale`), filtered
  to office/water_cave/icecave/outside: 31/31 match.
- final effects lab (`/tmp/step10-after-effects`): 29/29 match; neutral,
  visible and opaque pixel checks and live effect transitions pass.

All four captures have GL debug enabled, without GL or shader/material-layout
errors; only the existing synchronized pixel-readback performance warnings
appear. Linux headless game captures remain unavailable without gamescope.
Logs are `/tmp/step10-final-{linux-build,linux-ctest,windows-build}.log`,
`/tmp/step10-gl33.log`, `/tmp/step10-gl43.log` and
`/tmp/step10-{default,motion,scale,effects}-{capture,compare}.log`.

## Known issues

- Intermittent heap corruption on exit (`free(): corrupted unsorted chunks`).
- Physics may feel slightly different from the original (see above); not yet
  compared side by side.
