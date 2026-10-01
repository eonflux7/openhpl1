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
- Hardware gamma ramps removed (SDL3 has none). The gamma setting is stored
  but has no effect yet (see ROADMAP).

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

### Known issues

- Intermittent heap corruption on exit (`free(): corrupted unsorted chunks`).
- Gamma setting has no effect.
- Physics may feel slightly different from the original (see above); not yet
  compared side by side.
- Mouse sensitivity scales with resolution (original behaviour).
- Windows build not set up yet.
