# Audio

The engine talks to OpenAL Soft directly through `cOpenALDevice`
(`engine/{include,sources}/impl/OpenALDevice.*`). It uses the system's OpenAL
headers, so the full OpenAL Soft extension set is available at compile time.
Frictional Games' OALWrapper was removed on 2026-11-10; see
[the review](audio-review.md) for why.

```
cSoundHandler / cMusicHandler / game        (unchanged)
  cLowLevelSoundOpenAL, cOpenALSoundChannel,
  cOpenALSoundData, cOpenALSoundEnvironment  (adapters to the interfaces)
    cOpenALDevice   context, source pool, stream worker, reverb slot, filters
    cOpenALSample   Ogg (vorbisfile) or WAV (SDL3) decoded into one buffer
    cOpenALStream   Ogg decoded block by block into a buffer queue
```

## Device

- The `DeviceName` setting picks the output. An empty name or `NULL` (the
  shipped default) opens the default device. An unknown name falls back to
  the default device. The options menu switches devices at once with
  `alcReopenDeviceSOFT` and refreshes the device list on each click.
- `ALC_FREQUENCY` is not requested; OpenAL Soft mixes at the device's rate.
- The engine computes distance gains itself, so the distance model is
  `AL_NONE` and OpenAL is used for panning, pitch, reverb and filtering.
- `MaxSoundChannels` (32) sources are created up front. EFX
  (`UseEnvironmentalAudio`) adds two auxiliary slots with an EAX reverb each
  (standard reverb if EAX reverb is missing; one slot if the device has a
  single send), and one filter per source.

## Output: HRTF and device loss

- `[Sound] HRTF` is `auto` (default), `on` or `off`, also in the Sound
  options menu. Auto leaves the choice to OpenAL Soft, which enables HRTF
  when it detects headphones or its config says so. On asks for
  `ALC_HRTF_SOFT` and stereo HRTF output (`ALC_OUTPUT_MODE_SOFT`), even if
  the device reports surround. Off asks for no HRTF. The menu changes it
  with `alcResetDeviceSOFT`, without a restart; sources keep playing. The
  log and the menu show the result read back from `ALC_HRTF_STATUS_SOFT`.
- Stereo sounds (music streams and any stereo UI or ambience sample) use
  `AL_DIRECT_CHANNELS_SOFT` (with `AL_REMIX_UNMATCHED_SOFT` when present),
  so each channel goes to the matching output channel. OpenAL never
  positions stereo sources, so this changes nothing about placement; it
  keeps HRTF and surround layouts from playing them through virtual
  speakers. Mono sounds are positioned and get HRTF.
- `Update` (once per frame) reads `ALC_CONNECTED`. When the device is gone
  it logs once and calls `alcReopenDeviceSOFT` on the default device,
  retrying once a second. The context, sources, buffers and handles
  survive. A lost device stops every source: looping samples start again,
  streams restart through the underrun check and skip what was queued,
  and one-shot sounds that were cut off end. While disconnected the
  stream worker doesn't refill, so streams don't race ahead.
- A looping sample only counts as finished after `Stop`, so the sound
  handler doesn't drop looping ambience during a disconnect.
- No sample-rate setting: OpenAL Soft uses the device's rate.
  `AL_SOFT_source_spatialize` (positioning stereo world sounds) isn't used;
  no shipped stereo sound is known to need it.

## Sources and handles

The rules follow OALWrapper's, which the sound handler depends on:

- A handle packs the source index (12 bits) with a generation (19 bits).
  Binding new data or stealing a source bumps the generation, so stale
  handles are ignored, and `IsPlaying` and `GetPriority` return false and 0.
- Each channel of the data takes one voice; the voice budget is the number
  of sources. A request steals the lowest-priority sources *below* its own
  priority until enough voices are free. Sample priorities are 0–255;
  streams are 256, so samples cannot steal them.
- Sounds are created paused and count as playing until stopped or finished.
  A finished source keeps its data, and a stream stays bound, until the
  channel is stopped, which the sound handler does when it deletes the
  channel. When voices run short, finished sources are reclaimed (their
  handles go stale) before any playing source is stolen.
- Unpausing replays nothing that already ended: a one-shot sample that
  finished before the pause stays finished. Looping samples stopped by a
  device loss resume, as do paused streams.
- Gains are clamped to (0, 1] and pitch to (0, 10], as before.
- `SetFiltering(enabled, flags)` attaches the source's low-pass filter to
  the direct path (bit 1) and the reverb send (bit 2). Filter parameter
  changes are reapplied to the source, because OpenAL copies a filter when
  it is attached.

## Streams and threading

`StreamBufferSize` values below 4096 are KiB: the shipped `64` means 64 KiB
blocks. The old wrapper ignored values below 4096 bytes and streamed in 4 KiB
blocks (~93 ms queued), which underran on PipeWire. Blocks are 16 KiB to
1 MiB, with `StreamBufferCount` 3–16 of them (default 4 × 64 KiB, about 1.5 s
of 44.1 kHz stereo).

With `UseThreading` (default) a worker refills streams every
1000/`StreamUpdateFreq` ms, clamped to 5–50 ms. Otherwise refills happen in
`UpdateSound` on the main thread. Looping streams rewind the decoder; an
underrun restarts the source once buffers are queued again.

One `std::mutex` guards every source and stream. Every public source
operation and each worker pass takes it, and locking never depends on
whether the worker runs. The worker's stop flag is `std::atomic<bool>`, and
it waits on a condition variable. `Close` stops and joins the worker before
stopping sources and destroying the context. The old wrapper took its locks
only while a plain `bool` thread flag was set and cleared that flag during
shutdown (the EFX slot crashes of 2026-10-19 and the same pattern in its
stream manager).

Data is released before its buffers are deleted: `cOpenALSoundData` asks the
device to stop every source using it.

## Occlusion

`cSoundHandler` casts a ray from each blockable 3D sound to the listener
every 0.5 s (`kBlockRayInterval`), and at once when the sound starts. A hit
fades the entry's block multiplier to 0 over 0.55 s and a clear ray fades it
back to 1 over 0.2 s; at the first update it jumps. The multiplier lowers
the volume as before (toward the sound's `BlockVolumeMul` and a squared
distance curve), and it now also drives a low-pass on the direct path and
the reverb send:

    GainHF = OcclusionGainHF + block * (1 - OcclusionGainHF)

The filter is detached when the sound is clear, and only changes are sent
to the device. `[Sound] OcclusionGainHF` (default 0.25, at OpenAL's 5 kHz
reference) sets the fully blocked value; 1 turns filtering off. Rendered
white noise at 0.25 loses about 22 dB between 8 and 14 kHz relative to
200–800 Hz.

The original game cast the ray every 30 updates and enabled a filter with
unit gains when blocked and never disabled it, so blocked sounds only got
quieter. At the default 60 logic updates per second the ray interval is
unchanged. `cSoundHandler::SetSpeed`'s rate is now per second (nothing in
the games calls it).

Air absorption isn't used. OpenAL Soft does apply `AL_AIR_ABSORPTION_FACTOR`
with the engine's `AL_NONE` distance model (it uses the source's distance
past `AL_REFERENCE_DISTANCE`, 1 m by default): factor 10 at 30 m removed
about 26 dB above 8 kHz. A realistic factor of 1 is about −1.6 dB at 5 kHz
at 30 m, which game sounds' max distances rarely reach.

## Volume buses

The Sound menu's volume (`[Sound] Volume`) is the master: the listener gain.
Under it, four buses scale sounds by kind, each with a menu row and a
setting (0–1, default 1):

| Bus | Setting | Sounds |
|---|---|---|
| Music | `MusicVolume` | everything `cMusicHandler` plays, and handler sounds under `music/` |
| Voice | `VoiceVolume` | files under `sounds/voices/` (the intro story, Black Plague's radio) |
| Ambience | `AmbienceVolume` | any other looping sound: machinery, wind, water, pre-menu rain |
| Effects | `EffectsVolume` | everything else |

`cSoundHandler::BusForSound` picks the bus from the file path the sound
manager resolved (now kept in `iResourceBase::GetFilePath`) and whether the
sound loops; the path match is case-insensitive and folder-based, so a file
named `musicbox` is still an effect. The bus gain is a multiplier in the
handlers, applied with every volume they send, so a change takes effect at
the next update with sounds playing; OpenAL only sees the product. Game
logic doesn't see it: enemy hearing uses sound-entity data volumes, saves
store song volumes before the bus, and the pre-menu raindrops follow the
rain entry's volume rather than its channel's. `cSound::SetBusVolume` sets
the music bus in both handlers.

## Reverb

Sector reverb is used by Black Plague and Requiem (`soundenvs/*.snv`);
Overture's maps set no environment. Environments (`Type="OpenAL"`) are read
into `cOpenALSoundEnvironment`; malformed files are rejected without
crashing.

Every world sound sends to two reverb slots (send 0 to slot 0, send 1 to
slot 1, both through the occlusion filter). Each slot holds one
environment's parameters, loaded only when that environment isn't already
in a slot, and fades move only the slot gains:

    source slot: cos(t * pi/2)    destination slot: sin(t * pi/2)

times the environment volume (`SetEnvVolume`). A fade loads the destination
into the slot the source isn't using; the game only ever fades between the
environment playing and the next, so that slot is silent when it is
reloaded. No environment means both slots at gain 0; fading from or to
nothing ramps one slot. Before any environment is set the slots are silent.
With a single send, the one slot fades the source out and the destination
in, switching at the silent midpoint. The slot copies the effect when it is
attached, so loading an environment reattaches it.

The curve is equal-power because two different environments' reverbs are
uncorrelated. Rendered on a loopback device with noise and shipped preset
pairs (Room/Hallway, Room/Bathroom, Hallway/Vent, Room/DustyRoom,
StoneCorridor/Hallway, Room/Psychotic, Bathroom/Hallway), the correlation
was within ±0.02 and the midpoint level within 0.1 dB of the mean of the
two rooms; a linear fade dipped 3 dB. Two slots with identical early
reflections would correlate and rise about 3 dB at the midpoint, but the
shipped presets differ in density and delays.

The previous code interpolated every parameter between the environments,
toward zero for a missing one, and re-sent the whole effect each frame. At
the midpoint that was 0 to −1 dB from the mean for most pairs and −5 dB for
Hallway→Vent, and it moved through parameter sets neither room has. It
also never set `DecayHFRatio` on its fade environment, so each fade sent an
indeterminate value.

`ReflectionsPan` and `LateReverbPan` are now sent (EAX reverb only),
shortened to length 1 when longer. Of the 108 shipped presets, 106 have
zero pans; `Vent` (used in six maps) parses to small pans, and `Jensy` (one
map) has strong ones. The original code read them and never sent them, so
those two now sound as authored. `Vent`'s file writes negative components as
`0.-300000`, which parse as 0, as they always did.

## Music fades

`cMusicHandler` ramps a song's volume at a rate per second; saves and the
debug overlay read that volume. Fades in from silence and fades out are
shaped on output only: the stream gain is `ref*sin(pi/2 * volume/ref)`,
where `ref` is the target when a new song starts, or the gain heard when a
song starts fading out. Two songs crossfading at the same rate therefore
keep constant power instead of dipping 3 dB in the middle. When a playing
song gets a new target partway through a fade in, its volume becomes the
gain heard and the ramp continues linearly, so the gain never jumps. A song
at its target plays at exactly its volume, as before. Sound-entity fades
are single sounds, not crossfades, and stay linear.

## Sound variants

A sound name without a trailing digit picks a random variant (`name1`,
`name2`, ...). The handler probed `name1` upward through the resource
manager on every play, ending each time with a failed file search. It now
counts the variants once per base name and keeps the count; the chosen
variant is still loaded through the sound manager, so data a map change
unloaded is reloaded as before. Files added while the game runs aren't
seen until restart.

## Tests

`audio_regressions` runs with `ALSOFT_DRIVERS=null` under CTest and needs no
game data or audio device. It covers:

- twelve open/close cycles alternating the stream worker, with a looping
  stream, a filtered sample and reverb active, then closes while the worker
  is mid-update
- voice stealing: equal priority, stereo taking two voices, streams,
  finished sources reclaimed before a playing one is stolen
- stale handles, start-paused sounds and sounds finishing, and an ended
  sample staying ended through a pause and unpause
- stream refill past the queued blocks, end of a non-looping stream, looping
  past the length, and rebinding, with and without the worker
- Ogg and WAV decoding, float WAV conversion, and rejection of broken files
- sound-environment XML edge cases and fade interpolation
- `StreamBufferSize` units and the `NULL` device name
- HRTF on at open, and on/off/auto at runtime with a stream and a looping
  sample playing, read back through `ALC_HRTF_STATUS_SOFT`
- `alcReopenDeviceSOFT`: handles stay valid, the looping stream keeps
  refilling and the looping sample keeps playing; an unknown device fails
  and keeps the current one
- reading and writing the `HRTF` setting
- the occlusion low-pass: off when clear, blocked GainHF on the direct path
  and send when blocked, following the block fade, detached when clear,
  off when `OcclusionGainHF` is 1, and reset on a reused source
- bus rules (voices, music, looping, folder-only matching, backslashes) and
  routing: `cSoundHandler::Update` on real channels scales each entry by
  its bus only, mutes a bus at 0 without stopping it, and clamps gains
- reverb crossfades through `cLowLevelSoundOpenAL` with two environments,
  reading slot gains back from OpenAL: silent before an environment, a
  ramp from nothing, equal-power through a crossfade with the source
  keeping its slot, fading back without reloading, fading to nothing, the
  environment volume, and a world sound sending to both slots
- the music fade curve: constant power through a symmetric crossfade, no
  jump when a fade in gets a new target or turns into a fade out

A real unplug can't be simulated on the null backend; the reconnect path
is checked by hand.

The fixture `tests/data/tone_stereo_0.5s.ogg` is a 0.5 s 440 Hz tone made
with ffmpeg's libvorbis encoder. The tests don't check audible reverb,
the blocking rays or the mix; those need listening in the game. The
crossfade levels above were measured once with a throwaway loopback
program, not in CTest.
