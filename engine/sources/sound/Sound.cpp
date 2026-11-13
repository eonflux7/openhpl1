// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "sound/Sound.h"
#include "system/LowLevelSystem.h"
#include "sound/LowLevelSound.h"
#include "resources/Resources.h"
#include "sound/SoundHandler.h"
#include "sound/MusicHandler.h"


namespace hpl {

	cSound::cSound(iLowLevelSound *apLowLevelSound) : iUpdateable("HPL_Sound")
	{
		mpLowLevelSound = apLowLevelSound;
	}

	cSound::~cSound()
	{
		Log("Exiting Sound Module\n");
		Log("--------------------------------------------------------\n");

		delete mpSoundHandler;
		delete mpMusicHandler;

		Log("--------------------------------------------------------\n\n");
	}

	void cSound::Update(float afTimeStep)
	{
		mpSoundHandler->Update(afTimeStep);
		mpMusicHandler->Update(afTimeStep);

		mpLowLevelSound->UpdateSound(afTimeStep);
	}

	void cSound::SetBusVolume(eSoundBus aBus, float afVolume)
	{
		mpSoundHandler->SetBusVolume(aBus, afVolume);
		if(aBus == eSoundBus_Music) mpMusicHandler->SetBusVolume(afVolume);
	}

	float cSound::GetBusVolume(eSoundBus aBus)
	{
		return mpSoundHandler->GetBusVolume(aBus);
	}

	void cSound::Init(	cResources *apResources, bool abUseHardware, bool abForceGeneric, bool abUseEnvAudio, int alMaxChannels,
						int alStreamUpdateFreq, bool abUseThreading, bool abUseVoiceManagement,
						int alMaxMonoSourceHint, int alMaxStereoSourceHint,
						int alStreamingBufferSize, int alStreamingBufferCount, bool abEnableLowLevelLog, tString asDeviceName)
	{
		mpResources = apResources;

		Log("Initializing Sound Module\n");
		Log("--------------------------------------------------------\n");

		mpLowLevelSound->Init(	abUseHardware, abForceGeneric, abUseEnvAudio, alMaxChannels, alStreamUpdateFreq, abUseThreading,
								abUseVoiceManagement, alMaxMonoSourceHint, alMaxStereoSourceHint,
								alStreamingBufferSize, alStreamingBufferCount, abEnableLowLevelLog, asDeviceName);

		mpSoundHandler = new cSoundHandler(mpLowLevelSound, mpResources);
		mpMusicHandler = new cMusicHandler(mpLowLevelSound, mpResources);

		Log("--------------------------------------------------------\n\n");
	}
}
