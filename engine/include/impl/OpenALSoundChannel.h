// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_OPENAL_SOUND_CHANNEL_H
#define HPL_OPENAL_SOUND_CHANNEL_H

#include "sound/SoundChannel.h"
#include "sound/SoundData.h"

namespace hpl {

	class cOpenALDevice;

	class cOpenALSoundChannel : public iSoundChannel
	{
	public:
		cOpenALSoundChannel(iSoundData* apData, cOpenALDevice* apDevice, int alHandle, cSoundManager* apSoundManger);
		~cOpenALSoundChannel() override;

		void Play() override;
		void Stop() override;

		void SetPaused(bool abX) override;
		void SetSpeed(float afSpeed) override;
		void SetVolume (float afVolume) override;
		void SetLooping (bool abLoop) override;
		void SetPan (float afPan) override;
		void Set3D(bool ab3D) override;

		void SetPriority(int alX) override;
		int GetPriority() override;

		void SetPositionRelative(bool abRelative) override;
		void SetPosition(const cVector3f &avPos) override;
		void SetVelocity(const cVector3f &avVel) override;

		void SetMinDistance(float fMin) override;
		void SetMaxDistance(float fMax) override;

		bool IsPlaying() override;

		bool IsBufferUnderrun() override { return false; }
		double GetElapsedTime() override;
		double GetTotalTime() override;

		void SetAffectedByEnv(bool abAffected) override;
		void SetFiltering ( bool abEnabled, int alFlags) override;
		void SetFilterGain(float afGain) override;
		void SetFilterGainHF(float afGainHF) override;

	private:
		cOpenALDevice* mpDevice;
		int mlHandle;
	};
};
#endif // HPL_OPENAL_CHANNEL_H
