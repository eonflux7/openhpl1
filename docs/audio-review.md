# Audio review and modernization plan

Review of the audio stack on `feat/better-audio`, based on main at 4b105f7.
Covers `engine/sources/impl/*OpenAL*`, `engine/sources/sound/`,
`engine/sources/resources/SoundManager.cpp`, the game's music/sound-environment
handlers and the vendored OALWrapper (`vendor/oalwrapper`, about 6.6k lines).
Nothing below is implemented or measured yet. Line references point to the
tree at that commit.

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

## Suggested order of work

1. Defect 1 with a regression test, then defects 5, 7 and 8 (small and
   self-contained).
2. Defect 2, then remove the 50 Hz workaround.
3. System headers (modernization 1), HRTF/output mode and hot-plug.
4. Occlusion low-pass and time-based blocking (defects 3 and 4). Compare
   these by listening against the original game.
5. Reverb slot-gain crossfade (defect 6), then decide on equal-power fades
   and volume buses.
