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
  the default device.
- `ALC_FREQUENCY` is not requested; OpenAL Soft mixes at the device's rate.
- The engine computes distance gains itself, so the distance model is
  `AL_NONE` and OpenAL is used for panning, pitch, reverb and filtering.
- `MaxSoundChannels` (32) sources are created up front. EFX
  (`UseEnvironmentalAudio`) adds an EAX reverb in one auxiliary slot, falling
  back to standard reverb, and one filter per source.

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
  channel.
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

## Reverb

Sound environments (`.snt`, `Type="OpenAL"`) are read into
`cOpenALSoundEnvironment`; malformed files are rejected without crashing. A
missing environment is every parameter at zero, clamped to EAX's minimums,
which silences the reverb. Fades interpolate every parameter between the two
environments (a missing one counts as zero). The previous code never set
`DecayHFRatio` on its fade environment, which had uninitialized fields, so
each fade sent an indeterminate value. The slot copies the effect when it is
attached, so every change reattaches it.

## Tests

`audio_regressions` runs with `ALSOFT_DRIVERS=null` under CTest and needs no
game data or audio device. It covers:

- twelve open/close cycles alternating the stream worker, with a looping
  stream, a filtered sample and reverb active, then closes while the worker
  is mid-update
- voice stealing: equal priority, stereo taking two voices, streams
- stale handles, start-paused sounds and sounds finishing
- stream refill past the queued blocks, end of a non-looping stream, looping
  past the length, and rebinding, with and without the worker
- Ogg and WAV decoding, float WAV conversion, and rejection of broken files
- sound-environment XML edge cases and fade interpolation
- `StreamBufferSize` units and the `NULL` device name

The fixture `tests/data/tone_stereo_0.5s.ogg` is a 0.5 s 440 Hz tone made
with ffmpeg's libvorbis encoder. The tests don't check audible reverb,
occlusion or the mix; those need listening in the game.
