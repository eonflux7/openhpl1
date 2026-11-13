// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "sound/MusicHandler.h"

#include <cmath>

#include "resources/Resources.h"
#include "system/LowLevelSystem.h"
#include "system/String.h"
#include "math/Math.h"
#include "sound/LowLevelSound.h"
#include "sound/SoundChannel.h"
#include "sound/SoundData.h"
#include "resources/SoundManager.h"


namespace hpl {

	cMusicHandler::cMusicHandler(iLowLevelSound* apLowLevelSound, cResources* apResources)
	{
		mpLowLevelSound = apLowLevelSound;
		mpResources = apResources;

		mpMainSong = nullptr;
		mpLock  = nullptr;
		mbIsPaused = false;
		mfBusVolume = 1;
	}

	cMusicHandler::~cMusicHandler()
	{
		if(mpMainSong){
			hplDelete(mpMainSong->mpStream);
			hplDelete(mpMainSong);
		}

		tMusicEntryListIt it = mlstFadingSongs.begin();
		while(it != mlstFadingSongs.end())
		{
			cMusicEntry* pSong = *it;
			hplDelete(pSong->mpStream);
			hplDelete(pSong);

			it = mlstFadingSongs.erase(it);
		}
	}

	bool cMusicHandler::Play(const tString& asFileName,float afVolume, float afVolumeRatePerSecond, bool abLoop)
	{
		bool bSongIsPlaying = false;

		if(mpLock!=nullptr){
			mpLock->msFileName = asFileName;
			mpLock->mfVolume = afVolume;
			mpLock->mbLoop = abLoop;
			return true;
		}

		if(mpMainSong != nullptr)
			if(asFileName == mpMainSong->msFileName) bSongIsPlaying = true;

		if(!bSongIsPlaying)
		{
			//Put the previous song in the fading queue
			if(mpMainSong != nullptr)
			{
				BeginFadeOut(mpMainSong);
				mpMainSong->mfVolumeRatePerSecond = afVolumeRatePerSecond;
				mlstFadingSongs.push_back(mpMainSong);
			}

			//If there the song to be played is in the fade que, stop it.
			tMusicEntryListIt it = mlstFadingSongs.begin();
			while(it != mlstFadingSongs.end())
			{
				cMusicEntry* pSong = *it;
				if(pSong->msFileName == asFileName)
				{
					pSong->mfVolume= 0;
					pSong->mpStream->Stop();
					hplDelete(pSong->mpStream);
					hplDelete(pSong);

					it = mlstFadingSongs.erase(it);
				} else {
					it++;
				}
			}


			//add it and set its properties
			mpMainSong = hplNew( cMusicEntry, () );

			if(LoadAndStart(asFileName, mpMainSong,0,abLoop)==false){
				hplDelete(mpMainSong);
				mpMainSong = nullptr;
				return false;
			}
			// Fading in from silence: equal-power up to the target.
			mpMainSong->mfCurveRef = afVolume > 0 ? afVolume : 0;
		}
		else
		{
			mpMainSong->mbLoop = abLoop;
			mpMainSong->mpStream->SetLooping(abLoop);
			if(mpMainSong->mfMaxVolume == afVolume)return true;
			// A new target: carry on linearly from the gain heard now.
			EndCurve(mpMainSong);
		}

		//Set Properties
		mpMainSong->mfMaxVolume = afVolume;
		mpMainSong->mbLoop = abLoop;

		if(mpMainSong->mfMaxVolume > mpMainSong->mfVolume)
			mpMainSong->mfVolumeRatePerSecond = afVolumeRatePerSecond;
		else
			mpMainSong->mfVolumeRatePerSecond = -afVolumeRatePerSecond;


		return true;
	}

	void cMusicHandler::Stop(float afVolumeRatePerSecond)
	{
		if(mpMainSong==nullptr)return;

		if(afVolumeRatePerSecond<0)afVolumeRatePerSecond=-afVolumeRatePerSecond;

		BeginFadeOut(mpMainSong);
		mpMainSong->mfVolumeRatePerSecond = afVolumeRatePerSecond;
		if(afVolumeRatePerSecond==0){
			mpMainSong->mpStream->SetVolume(0);
			mpMainSong->mpStream->Stop();
			mpMainSong->mfVolume =0;
		}

		mlstFadingSongs.push_back(mpMainSong);
		mpMainSong = nullptr;
	}

	void cMusicHandler::Pause()
	{
		if(mpMainSong != nullptr)mpMainSong->mpStream->SetPaused(true);

		tMusicEntryListIt it = mlstFadingSongs.begin();
		while(it != mlstFadingSongs.end()){
			(*it)->mpStream->SetPaused(true);
			it++;
		}

		mbIsPaused = true;
	}

	void cMusicHandler::Resume()
	{
		if(mpMainSong != nullptr)mpMainSong->mpStream->SetPaused(false);

		tMusicEntryListIt it = mlstFadingSongs.begin();
		while(it != mlstFadingSongs.end()){
			(*it)->mpStream->SetPaused(false);
			it++;
		}

		mbIsPaused = false;
	}

	void cMusicHandler::Lock(cMusicLock* apLock)
	{
		mpLock = apLock;
	}

	void cMusicHandler::UnLock()
	{
		mpLock = nullptr;
	}

	tString cMusicHandler::GetCurrentSongName()
	{
		if(mpMainSong!=nullptr)
			return mpMainSong->msFileName;
		else
			return "";
	}

	float cMusicHandler::GetCurrentSongVolume()
	{
		if(mpMainSong!=nullptr)
			return mpMainSong->mfVolume;
		else
			return 0;
	}

	cMusicEntry* cMusicHandler::GetCurrentSong()
	{
		return mpMainSong;
	}

	void cMusicHandler::Update(float afTimeStep)
	{
		if(mbIsPaused)return;

		if(mpMainSong != nullptr)
		{
			if(mpMainSong->mpStream->IsPlaying()==false)
			{
				hplDelete(mpMainSong->mpStream);
				hplDelete(mpMainSong);
				mpMainSong = nullptr;
			}
			else
			{
				//Update the main song
				mpMainSong->mfVolume+=mpMainSong->mfVolumeRatePerSecond*afTimeStep;

				if(mpMainSong->mfVolumeRatePerSecond>0)
				{
					if(mpMainSong->mfVolume>=mpMainSong->mfMaxVolume)
						mpMainSong->mfVolume= mpMainSong->mfMaxVolume;
				}
				else
				{
					if(mpMainSong->mfVolume<=mpMainSong->mfMaxVolume)
						mpMainSong->mfVolume= mpMainSong->mfMaxVolume;
				}


				float fGain = FadeGain(mpMainSong) * mfBusVolume;
				if(mpMainSong->mpStream->GetVolume()!=fGain)
				{
					mpMainSong->mpStream->SetVolume(fGain);
				}
			}
		}


		//Update the fading songs
		tMusicEntryListIt it = mlstFadingSongs.begin();
		while(it != mlstFadingSongs.end())
		{
			cMusicEntry* pSong = *it;
			pSong->mfVolume-=pSong->mfVolumeRatePerSecond*afTimeStep;

			if(pSong->mfVolume<=0)
			{
				pSong->mfVolume= 0;
				pSong->mpStream->Stop();
				hplDelete(pSong->mpStream);
				hplDelete(pSong);

				it = mlstFadingSongs.erase(it);
			}
			else
			{
				pSong->mpStream->SetVolume(FadeGain(pSong) * mfBusVolume);
				it++;
			}
		}
	}

	void cMusicHandler::SetBusVolume(float afVolume)
	{
		mfBusVolume = cMath::Clamp(afVolume, 0.0f, 1.0f);
	}

	float cMusicHandler::FadeGain(const cMusicEntry *apSong)
	{
		float fRef = apSong->mfCurveRef;
		if(fRef <= 0) return apSong->mfVolume;
		float fT = cMath::Clamp(apSong->mfVolume / fRef, 0.0f, 1.0f);
		return fRef * std::sin(fT * kPi2f);
	}

	// Continue linearly from the current gain, so there is no jump.
	void cMusicHandler::EndCurve(cMusicEntry *apSong)
	{
		apSong->mfVolume = FadeGain(apSong);
		apSong->mfCurveRef = 0;
	}

	// Fading out to silence: the curve starts from the current gain.
	void cMusicHandler::BeginFadeOut(cMusicEntry *apSong)
	{
		EndCurve(apSong);
		apSong->mfCurveRef = apSong->mfVolume;
	}

	bool cMusicHandler::LoadAndStart(const tString& asFileName,cMusicEntry* apSong  ,float afVolume, bool abLoop)
	{
		iSoundData* pData = mpResources->GetSoundManager()->CreateSoundData(asFileName,true,abLoop);
		if(pData==nullptr){
			Error("Couldn't load music '%s'\n",asFileName.c_str());
			return false;
		}

		iSoundChannel *pStream = pData->CreateChannel(256);
		if(pStream == nullptr){
			Error("Couldn't stream music '%s'!\n",asFileName.c_str());
			return false;
		}

		apSong->msFileName = asFileName;
		apSong->mpStream = pStream;
		apSong->mpStream->SetVolume(afVolume * mfBusVolume);
		apSong->mpStream->SetLooping(abLoop);

		apSong->mpStream->Play();

		return true;
	}
}
