#include <iostream>
#include <new>
#include <stdexcept>
#include <thread>

#include <SDL3/SDL.h>
#include "OALWrapper/OAL_Device.h"
// Check real slot lock ownership as well as successful device startup.
#define private public
#include "OALWrapper/OAL_EffectSlot.h"
#undef private

extern cOAL_Device* gpDevice;

static void Check(bool abResult, const char* asMessage)
{
	if(!abResult) throw std::runtime_error(asMessage);
}

static void TestSlotMutex(cOAL_EffectSlot* apSlot)
{
	Check(apSlot && apSlot->mpMutex, "EFX slot owns a mutex before updater startup");
	apSlot->Lock();
	bool bOtherThreadAcquired = false;
	std::thread contender([&]()
	{
		bOtherThreadAcquired = SDL_TryLockMutex(apSlot->mpMutex);
		if(bOtherThreadAcquired) SDL_UnlockMutex(apSlot->mpMutex);
	});
	contender.join();
	apSlot->Unlock();
	Check(!bOtherThreadAcquired, "EFX slot excludes another thread");
	std::thread afterUnlock([&]()
	{
		bOtherThreadAcquired = SDL_TryLockMutex(apSlot->mpMutex);
		if(bOtherThreadAcquired) SDL_UnlockMutex(apSlot->mpMutex);
	});
	afterUnlock.join();
	Check(bOtherThreadAcquired, "EFX slot releases its mutex");
}

int main()
{
	try
	{
		Check(SDL_Init(0), "SDL initialization");
		// Exercise construction and teardown with and without the real updater.
		for(int i = 0; i < 12; ++i)
		{
			cOAL_Init_Params params;
			params.mbUseEFX = true;
			params.mbUseThread = (i % 2) != 0;
			params.mlNumSourcesHint = 2;
			params.mlNumSlotsHint = 2;
			params.mlSlotUpdateFreq = 1000;
			Check(OAL_Init(params), "null audio device initialization");
			Check(OAL_Info_IsEFXActive(), "null audio device supports EFX");
			cOAL_EFXManager* pManager = gpDevice->GetEFXManager();
			if(!params.mbUseThread)
			{
				// Zeroed recycled storage makes the old missing initialization
				// deterministic: a nonthreaded slot used to have no mutex.
				alignas(cOAL_EffectSlot) unsigned char storage[sizeof(cOAL_EffectSlot)] = {};
				cOAL_EffectSlot* pSlot = new(storage) cOAL_EffectSlot(pManager, 99);
				TestSlotMutex(pSlot);
				pSlot->~cOAL_EffectSlot();
			}
			for(int slot = 0; slot < pManager->GetNumSlots(); ++slot)
				TestSlotMutex(pManager->GetEffectSlot(slot));
			SDL_Delay(3);
			OAL_Close();
		}
		SDL_Quit();
		std::cout << "Audio regressions passed\n";
		return 0;
	}
	catch(const std::exception& error)
	{
		std::cerr << error.what() << '\n';
		OAL_Close();
		SDL_Quit();
		return 1;
	}
}
