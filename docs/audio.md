# Audio lifecycle

The engine uses the vendored OALWrapper with OpenAL Soft. EFX is enabled by
`Sound.UseEnvironmentalAudio` and the worker threads by `Sound.UseThreading`;
both default to true. Null audio still exercises EFX and its worker.

Each `cOAL_EffectSlot` owns an SDL mutex from construction until destruction,
including when the updater is disabled. Locking must not depend on the
manager's current worker flag: slots are constructed before the worker starts,
and a worker already holding a slot must unlock it after a stop request.
The EFX manager initializes its lifecycle fields in its constructor and uses
an atomic worker flag. Shutdown requests a stop, joins the worker, then
destroys the slots and their mutexes.

The previous slot constructor read an uninitialized manager flag and could
leave the slot mutex uninitialized. Windows startup dumps from renderer
validation fault in `SDL_LockMutex_srw` while the EFX worker updates slots.
SDL3 3.4.8's fault RVA `0x166df5` maps to the read of `mutex->owner` in
`src/thread/windows/SDL_sysmutex.c:71`. The saved mutex arguments include
invalid values such as `0x735f5458455f4c47` and `0xffffffff`. Saved stack
return addresses map to `cOAL_EFXManager::UpdateSlots` and `SlotUpdaterThread`;
the dump that faults in `ntdll.dll` has the same EFX caller addresses.
This evidence identifies an application mutex lifetime bug rather than
establishing an SDL or graphics driver defect.

`audio_regressions` runs with `ALSOFT_DRIVERS=null` under CTest. It checks real
mutex exclusion and release, a slot constructed in zeroed storage while the
worker is disabled, and twelve device startup/teardown cycles alternating
threaded and nonthreaded EFX. The zeroed slot fixture exposes the old missing
initialization without depending on allocator contents. Normal and sanitizer
runs exercise the same code; leak detection disabled in a run does not
establish leak-free teardown. This coverage does not validate audible reverb,
stream playback, or all other audio ownership paths.
