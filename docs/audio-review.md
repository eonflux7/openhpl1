# Audio review and modernization plan

Review of the audio stack on `feat/better-audio`, based on main at 4b105f7.
Covers `engine/sources/impl/*OpenAL*`, `engine/sources/sound/`,
`engine/sources/resources/SoundManager.cpp`, the game's music/sound-environment
handlers and the vendored OALWrapper (`vendor/oalwrapper`, about 6.6k lines).
Line references point to the tree at that commit.

## Status (2026-11-10)

Decision: replace OALWrapper rather than patch it. The engine used about 70
of its functions, mostly through four adapter files, and its two worker
threads held both threading defects found so far. The reverb (EFX) thread
only reattached the effect, which the engine already did itself.

Done on this branch (see [audio](audio.md) and CHANGES):

- OALWrapper removed. `cOpenALDevice` talks to OpenAL Soft through the
  system headers (modernization 1), keeps OALWrapper's source, priority and
  stream rules, and uses one mutex that never depends on a flag (defect 1).
- Defect 2: `StreamBufferSize` is read as KiB (64 KiB × 4 blocks); the 50 Hz
  refill floor is gone.
- Defect 5: malformed sound-environment files are rejected safely.
- Defect 6 (partly): fades no longer send an uninitialized `DecayHFRatio`.
  The parameter-lerp fade itself is unchanged.
- Defect 7: `NULL` and empty device names open the default device without
  an error; `ALC_FREQUENCY` is no longer forced to 44.1 kHz.
- Defect 8: the leaked environment and the unused effect are gone. `SetPan`
  stays a no-op as before.
- Modernization 7: no `alGetError` after every call.

The remaining work and its order are in [Plan](#plan).

## Stack in brief

```
game (GameMusicHandler, GameSoundEnvHandler, SoundEntity)
  -> cSoundHandler / cMusicHandler      engine/sources/sound
  -> cLowLevelSoundOpenAL, cOpenALSoundChannel/Data/Environment   impl/
  -> OALWrapper (OAL_* C API, source manager + EFX worker threads, Ogg/WAV)
  -> OpenAL Soft 1.25 (system), libvorbisfile
```

The engine does its own audio math. It uses `eOAL_DistanceModel_None`
(`LowLevelSoundOpenAL.cpp:228`) and sets each source's gain every frame from
linear min/max-distance attenuation, a blocking ray and a fade multiplier. It
passes source positions to OpenAL only for panning. Gameplay depends on this
behaviour, so the plan below keeps the attenuation curves and moves the
plumbing underneath them.

## Defects (fix first)

1. **Streaming worker locks depend on a non-atomic flag that is cleared
   during shutdown.** This is the same class of bug that `docs/audio.md`
   describes for EFX slots. In `cOAL_SourceManager`:
   - `LockStreamList` and `UnlockStreamList` (`OAL_SourceManager.cpp:177-190`)
     and `cOAL_Source::Lock` and `Unlock` (`OAL_Source.cpp:153-160`) take a
     mutex only while `mbUseThreading` is true.
   - `Destroy()` (`OAL_SourceManager.cpp:114-119`) sets
     `mbUseThreading = false` (a plain `bool`) while the updater may already
     hold the list mutex and a source mutex. The worker then skips its
     unlocks. The stream-list mutex is destroyed while locked, and the main
     thread's later `Lock`/`Unlock` calls on sources stop excluding the
     worker.
   - This is a plausible cause of the separate ROADMAP item
     "heap-corruption-on-exit report", but it has not been confirmed.

   Fix: follow the EFX fix. Mutexes live for the lifetime of their objects.
   Locking does not depend on the flag. The stop flag is `std::atomic<bool>`.
   Shutdown stops, joins, then destroys. Extend `audio_regressions` with a
   streaming source that is mid-update when the device closes.
2. **The configured stream buffer size is ignored.** The game passes the
   config default `StreamBufferSize=64` (`game/core/Init.cpp:614`).
   `cOAL_Stream::SetBufferSize` ignores values below 4096
   (`OAL_Stream.h:38`), so streams run with 4 KiB × 4 buffers, about 93 ms of
   44.1 kHz stereo. The 50 Hz refill floor in `LowLevelSoundOpenAL.cpp:189-191`
   works around the resulting underruns. Treat the value as KiB, or set a
   floor of about 250 ms of audio, then remove the polling-rate workaround.
3. **Occlusion filtering is never undone and has no audible effect.**
   `UpdateDistanceVolume3D` calls `SetFiltering(true, 0xF)` when a ray is
   blocked (`SoundHandler.cpp:850`). The matching disable and the
   `SetFilterGainHF` calls are commented out. The filter keeps its default
   gains of 1.0, so blocked sounds only become quieter; the filter does
   nothing. Blocked sounds should get a low-pass filter (`GainHF`), and the
   filter should fade back out when the ray is clear.
4. **Frame-rate-dependent timing:**
   - The global speed fade adds `mfSpeedRate` once per frame without
     multiplying by `afTimeStep` (`cSoundHandler::Update`).
   - The blocking ray runs every 30 *updates* per entry (`mlCount % 30`). At
     high frame rates it reacts sooner; at 30 fps it takes a second.

   Use time accumulators. Check save/replay expectations first; the speed fade
   is used by slow-motion and effects scripts.
5. **The sound-environment XML parser can crash on malformed or foreign
   files.** `cOpenALSoundEnvironment::CreateFromFile` dereferences
   `doc.FirstChildElement("SoundEnvironment")` without a null check. It also
   assigns `Attribute("Type")` or `Attribute("Name")` to `std::string`, which
   is undefined behaviour when the attribute is missing.
6. **Reverb fades interpolate every parameter toward zero.**
   `SetSoundEnvironment(nullptr)` and the null branches of
   `FadeSoundEnvironment` scale fields such as `HFReference`, `EchoTime`,
   `DecayHFRatio` and `AirAbsorptionGainHF` toward 0. OALWrapper clamps these
   values, so OpenAL reports no errors. The fade still moves through a
   meaningless parameter set; only the gain change is audible. Better
   approach:
   - Keep the target environment's parameters fixed and fade the effect-slot
     gain (or the reverb gain).
   - For environment-to-environment fades, crossfade two slots. The device
     already has 4 sends.
   - Apply the loaded `ReflectionsPan` and `LateReverbPan` too; they are
     parsed but never sent.
7. **Device selection:**
   - The default `DeviceName` is the literal string `"NULL"`
     (`Init.cpp:592`). Every launch first fails to open a device named
     "NULL", logs an error, then retries with the default device. Map
     `"NULL"` and empty names to the default device without logging an
     error.
   - `ALC_FREQUENCY` is fixed at 44100 (`LowLevelSoundOpenAL.cpp:197`), which
     forces resampling on 48 kHz PipeWire/WASAPI devices. Leave it unset so
     OpenAL Soft uses the device's native rate.
8. **Minor leaks and dead code:**
   - `Init` allocates a `cOpenALSoundEnvironment` and never uses or frees it
     (`LowLevelSoundOpenAL.cpp:178`).
   - `mpEffect` is created even when EFX is unavailable. It is never
     destroyed explicitly.
   - Unused `fVel`.
   - `SetPan`, `SetPositionRelative` and the channel min/max distance are
     stubs. Non-3D world sounds call `SetPan` (`SoundHandler.cpp`, 2D branch)
     and get no panning.

## Modernization

Ordered by value for the risk involved.

1. **Use the system OpenAL headers.** The vendored `include/AL/*.h` date from
   about 2006 and shadow the system's OpenAL Soft 1.25 headers through the
   `SYSTEM PUBLIC` include path. As a result, `ALC_SOFT_HRTF`,
   `ALC_SOFT_output_mode`, `ALC_SOFT_reopen_device`,
   `AL_SOFT_direct_channels` and similar extensions are not visible. Delete
   the vendored copies, or move them behind a fallback used only when the
   system copies are missing. vcpkg's `openal-soft` provides the same headers
   on Windows.
2. **Spatial output options.** Expose `Sound.Hrtf` (auto, on or off) and an
   output mode for stereo, headphones and surround through
   `ALC_HRTF_SOFT` / `ALC_OUTPUT_MODE_SOFT` context attributes. Put these in
   the options menu next to the device list. Give non-positional music and UI
   streams `AL_DIRECT_CHANNELS_SOFT` so stereo music is not virtualised
   through HRTF.
3. **Device hot-plug and recovery.**
   - Poll `ALC_CONNECTED` (`ALC_EXT_disconnect`) each frame.
   - When the device disappears, call `alcReopenDeviceSOFT` with the default
     device. This keeps the context, sources and buffers.
   - Today, unplugging headphones silences the game until it restarts.
   - Allow the device to change at runtime from the options menu instead of
     requiring a restart.
4. **Replace the polling stream thread.**
   - Short term: fix defect 1, use larger buffers, and replace
     `SDL_Delay`-based polling with a condition variable or an
     `AL_SOFT_events` buffer-completed wakeup.
   - Longer term: OpenAL Soft's `AL_SOFT_callback_buffer` lets the mixer pull
     decoded PCM, which removes buffer queueing.
5. **Decode once per sound, not per variant probe.**
   `cSoundHandler::CreateChannel` probes `name1`, `name2` and so on through
   the file searcher every time a sound plays (`SoundHandler.cpp`, tail).
   Cache the variant count per base name.
   - Samples are decoded with 16-bit `ov_read`. Optionally decode to float
     (`AL_EXT_FLOAT32`) to keep headroom. The Ogg files ship as 16-bit, so
     this gains little.
   - Consider adding Opus or FLAC loaders only if mods need them.
6. **Gain staging and fades.**
   - Music crossfades (`cMusicHandler`) and sound-entity fades are linear in
     amplitude, which causes a perceptible dip in the middle of a crossfade.
     Equal-power curves would remove it. This is an audible change, so make
     it an explicit decision.
   - Add separate master, effects, music and voice volume buses. Today the
     options menu offers only master volume plus the engine's global
     `SetVolume` fade.
7. **Reduce error-check overhead.** `RUN_AL_FUNC` calls `alGetError` after
   every AL call, and each call takes OpenAL Soft's context lock. Compile the
   check out of release builds, or check only when logging is enabled.
8. **Longer-term option: replace OALWrapper.** About 6.6k lines of
   vendored, lightly maintained code sit between the engine and OpenAL.
   Once items 1-4 are done, a thin backend that calls OpenAL Soft directly
   (sources, buffers, EFX, an Ogg reader) would be smaller than the wrapper.
   Alternatively, miniaudio/SDL3 audio with a custom mixer would remove the
   OpenAL dependency but would require rewriting the reverb. Not
   recommended before the defects are fixed and covered by tests.

## Validation plan

- `audio_regressions` under `ALSOFT_DRIVERS=null`, normal and ASan/UBSan:
  - streaming-thread shutdown while a stream is mid-refill (defect 1)
  - malformed sound-environment XML (defect 5)
  - a stream that plays without underrunning at the default buffer settings,
    checked by counting `AL_BUFFERS_PROCESSED` stalls (defect 2)
- ASan game smoke (`OPENHPL_SMOKE=1`) for all three games, plus
  repeated quit-cycles to look for the exit heap corruption.
- Manual listening:
  - reverb transitions between sectors (Overture's mines; Black Plague's
    labs)
  - occluded sounds behind doors
  - music crossfades
  - HRTF on headphones
  - unplugging and replugging a device
- Windows build and smoke separately; MSVC does not run CTest.

## Plan

Agreed 2026-11-10, after the backend replacement. Steps change what
players hear from step 2 on, so each needs a listening comparison against
the original game as well as tests.

0. **Done:** the direct OpenAL Soft backend replaced OALWrapper, and
   defects 1, 2, 5, 7 and 8 are fixed (see [Status](#status-2026-11-10)).
1. **Done (code and tests; listening and a real unplug not yet checked):
   HRTF/output mode and device recovery** (modernization 2 and 3). See
   [audio](audio.md#output-hrtf-and-device-loss).
   - `[Sound] HRTF` (auto/on/off) in the settings and the Sound options
     menu, applied at runtime with `alcResetDeviceSOFT`. On also asks for
     stereo HRTF output (`ALC_OUTPUT_MODE_SOFT`). A separate output-mode
     option wasn't added: OpenAL Soft takes the layout from the device.
   - Every stereo source uses `AL_DIRECT_CHANNELS_SOFT`, not only music and
     UI: OpenAL doesn't position stereo sources, so direct channels only
     stop HRTF and surround layouts from virtualising them.
   - `ALC_CONNECTED` is checked each frame; on loss `alcReopenDeviceSOFT`
     moves to the default device, retried once a second. The menu's device
     option uses the same call instead of a restart.
   - No sample-rate setting, and `AL_SOFT_source_spatialize` isn't used.
2. **Occlusion** (defects 3 and 4).
   - Blocked sounds get a low-pass (`GainHF`) that fades in and out, and the
     filter turns off when the ray is clear.
   - Blocking rays and the global speed fade run on time, not per frame.
   - The raycast plus low-pass stays the default. Per-source air absorption
     comes after it, but first check whether OpenAL Soft applies it with
     the engine's `AL_NONE` distance model. Steam Audio only if measured CPU
     cost allows it and the quality gain is audible; it would add to EFX
     reverb, not replace it.
3. **Volume buses with options-menu sliders** (modernization 6).
   - Master, music, SFX, voice and ambience gains, as multipliers in the
     sound and music handlers rather than in OpenAL.
   - First decide how a sound gets its category: sound-entity data, the
     file path (`sounds/voice/…`), or how it is played (looping sound
     entities count as ambience).
   - Ducking and snapshots (menus, cutscenes, underwater) come later, and
     only where a scene needs one. The handlers already fade and pause by
     sound type.
4. **Reverb crossfades** (defect 6). EFX sector reverb stays: it is cheap,
   authored in the games' environment files and part of the original feel.
   Fade the slot gain, or crossfade two slots, instead of interpolating
   every parameter toward zero. Decide on equal-power music fades at the
   same time.

Also planned: cache sound-variant counts (modernization 5).

**On demand only** (no shipped asset needs them):
- FLAC or Opus, through `dr_flac` or `opusfile` rather than miniaudio or
  libsndfile.
- `AL_SOFT_loop_points` and ambisonic/B-format.
- Snapshots beyond those in step 3.

Not needed:
- A fixed 50 Hz refill rate. Stream stutter came from 4 KiB buffers. With
  4 × 64 KiB queued, the worker's 5–50 ms wake-ups leave about 1.5 s of
  headroom; the rule is to keep enough audio queued, not a refill rate.
- Streaming from `.zip` addons. They are unpacked to `cache/addons/`, so
  the decoders read them as ordinary files.

**Tests to add with each step** (`audio_regressions`, null backend):
- step 1: HRTF on/off, read back through `ALC_HRTF_STATUS_SOFT`; the reopen
  path, with sources surviving `alcReopenDeviceSOFT`. A real unplug can't
  be simulated on the null backend and is checked by hand.
- step 2: filter state when sounds are blocked and cleared.
- step 3: bus routing, in the engine's sound-handler tests rather than the
  backend's, since the buses are handler gains.
- step 4: reverb slot gains during a crossfade.

Stream continuity, source filtering and shutdown order are already covered.
