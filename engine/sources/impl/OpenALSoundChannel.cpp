/*
 * Copyright (C) 2006-2010 - Frictional Games
 *
 * This file is part of HPL1 Engine.
 *
 * HPL1 Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HPL1 Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HPL1 Engine.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "impl/OpenALSoundChannel.h"
#include "impl/OpenALDevice.h"
#include "resources/SoundManager.h"

namespace hpl {

	cOpenALSoundChannel::cOpenALSoundChannel(iSoundData* apData, cOpenALDevice* apDevice, int alHandle,
											 cSoundManager* apSoundManger)
	: iSoundChannel(apData, apSoundManger), mpDevice(apDevice), mlHandle(alHandle)
	{
		const float vZero[3] = {0,0,0};
		mpDevice->SetPositionVelocity(mlHandle, vZero, vZero);
		mpDevice->SetLowPassFilter(mlHandle, true);
	}

	cOpenALSoundChannel::~cOpenALSoundChannel()
	{
		if(mlHandle>=0)
			mpDevice->Stop(mlHandle);

		if(mpSoundManger)mpSoundManger->Destroy(mpData);
	}

	void cOpenALSoundChannel::Play()
	{
		SetPaused(false);

		mbStopUsed = false;
	}

	void cOpenALSoundChannel::Stop()
	{
		mpDevice->Stop(mlHandle);
		mlHandle = -1;

		mbStopUsed = true;
	}

	void cOpenALSoundChannel::SetPaused(bool abX)
	{
		mbPaused = abX;
		mpDevice->SetPaused(mlHandle, abX);
	}

	void cOpenALSoundChannel::SetSpeed(float afSpeed)
	{
		mfSpeed = afSpeed;
		mpDevice->SetPitch(mlHandle, afSpeed);
	}

	void cOpenALSoundChannel::SetVolume(float afVolume)
	{
		mfVolume = afVolume;
		mpDevice->SetGain(mlHandle, mfVolume);
	}

	void cOpenALSoundChannel::SetLooping (bool abLoop)
	{
		mbLooping = abLoop;
		mpDevice->SetLoop(mlHandle, abLoop);
	}

	void cOpenALSoundChannel::SetPan (float afPan)
	{
		mfPan = afPan;
	}

	void cOpenALSoundChannel::Set3D(bool ab3D)
	{
		mb3D = ab3D;
	}

	void cOpenALSoundChannel::SetPositionRelative(bool abRelative)
	{
		// The sound handler converts relative positions to world space itself.
		mbPositionRelative = abRelative;
	}

	void cOpenALSoundChannel::SetPosition(const cVector3f &avPos)
	{
		mvPosition = avPos;
		mpDevice->SetPositionVelocity(mlHandle, mvPosition.v, mvVelocity.v);
	}

	void cOpenALSoundChannel::SetVelocity(const cVector3f &avVel)
	{
		mvVelocity = avVel;
		mpDevice->SetPositionVelocity(mlHandle, mvPosition.v, mvVelocity.v);
	}

	void cOpenALSoundChannel::SetMinDistance(float afMin)
	{
		mfMinDistance = afMin;
	}

	void cOpenALSoundChannel::SetMaxDistance(float afMax)
	{
		mfMaxDistance = afMax;
	}

	bool cOpenALSoundChannel::IsPlaying()
	{
		return mpDevice->IsPlaying(mlHandle);
	}

	double cOpenALSoundChannel::GetElapsedTime()
	{
		return mpDevice->GetElapsedTime(mlHandle);
	}

	double cOpenALSoundChannel::GetTotalTime()
	{
		return mpDevice->GetTotalTime(mlHandle);
	}

	void cOpenALSoundChannel::SetPriority(int alX)
	{
		int lPrio = alX+mlPriorityModifier;
		if(lPrio>255)lPrio = 255;

		mpDevice->SetPriority(mlHandle, lPrio);
	}

	int cOpenALSoundChannel::GetPriority()
	{
		return mpDevice->GetPriority(mlHandle);
	}

	void cOpenALSoundChannel::SetAffectedByEnv(bool abAffected)
	{
		iSoundChannel::SetAffectedByEnv(abAffected);

		mpDevice->SetSendToReverb(mlHandle, mbAffectedByEnv);
	}

	void cOpenALSoundChannel::SetFiltering(bool abEnabled, int alFlags)
	{
		mpDevice->SetFiltering(mlHandle, abEnabled, alFlags);
	}

	void cOpenALSoundChannel::SetFilterGain(float afGain)
	{
		mpDevice->SetFilterGain(mlHandle, afGain);
	}

	void cOpenALSoundChannel::SetFilterGainHF( float afGainHF)
	{
		mpDevice->SetFilterGainHF(mlHandle, afGainHF);
	}

}
