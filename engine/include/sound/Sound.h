// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_SOUND_H
#define HPL_SOUND_H

#include "game/Updateable.h"

namespace hpl {

	/** Volume groups set from the options menu. Master is the listener gain
	 * (iLowLevelSound::SetVolume); these multiply under it. */
	enum eSoundBus
	{
		eSoundBus_Effects,
		eSoundBus_Ambience,
		eSoundBus_Voice,
		eSoundBus_Music,
		eSoundBus_LastEnum
	};

	/** Settings keys for each bus, e.g. [Sound] MusicVolume. */
	inline const char *const gvSoundBusConfigNames[eSoundBus_LastEnum] = {
		"EffectsVolume", "AmbienceVolume", "VoiceVolume", "MusicVolume" };

	class iLowLevelSound;
	class cResources;
	class cSoundHandler;
	class cMusicHandler;

	class cSound : public iUpdateable
	{
	public:
		cSound(iLowLevelSound *apLowLevelSound);
		~cSound() override;

		void Init(	cResources *apResources, bool abUseHardware, bool abForceGeneric, bool abUseEnvAudio, int alMaxChannels,
						int alStreamUpdateFreq, bool abUseThreading, bool abUseVoiceManagement,
						int alMaxMonoSourceHint, int alMaxStereoSourceHint,
						int alStreamingBufferSize, int alStreamingBufferCount, bool abEnableLowLevelLog, tString asDeviceName);

		void Update(float afTimeStep) override;

		iLowLevelSound* GetLowLevel(){ return mpLowLevelSound;}
		cSoundHandler* GetSoundHandler(){ return mpSoundHandler; }
		cMusicHandler* GetMusicHandler(){ return mpMusicHandler; }

		/** Sets a bus gain (0-1) in both handlers; it applies at the next update. */
		void SetBusVolume(eSoundBus aBus, float afVolume);
		float GetBusVolume(eSoundBus aBus);

	private:
		iLowLevelSound *mpLowLevelSound;
		cResources* mpResources;
		cSoundHandler* mpSoundHandler;
		cMusicHandler* mpMusicHandler;
	};

};
#endif // HPL_SOUND_H
