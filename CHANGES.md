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

## Known issues

- Intermittent heap corruption on exit (`free(): corrupted unsorted chunks`).
- Physics may feel slightly different from the original (see above); not yet
  compared side by side.
- Windows build not set up yet.
