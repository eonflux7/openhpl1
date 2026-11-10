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
#ifndef HPL_SOUNDHANDLER_H
#define HPL_SOUNDHANDLER_H

#include <list>
#include "system/SystemTypes.h"
#include "math/MathTypes.h"
#include "physics/PhysicsWorld.h"

namespace hpl {

	class iLowLevelSound;
	class iSoundChannel;
	class cWorld3D;

	//----------------------------------------

	enum eSoundDest
	{
		eSoundDest_World = eFlagBit_0,
		eSoundDest_Gui = eFlagBit_1,
		eSoundDest_All = eFlagBit_All
	};

	//----------------------------------------

	class cSoundRayCallback :public iPhysicsRayCallback
	{
	public:
		void Reset();
		bool HasCollided(){ return mbHasCollided;}

		bool BeforeIntersect(iPhysicsBody *pBody) override;
		bool OnIntersect(iPhysicsBody *pBody,cPhysicsRayParams *apParams) override;

	private:
		bool mbHasCollided;
		int mlCount;
	};

	//----------------------------------------


	////////////////////////////////////////////////////
	//////////// SOUND ENTRY ///////////////////////////
	////////////////////////////////////////////////////

	class cSoundEntry
	{
	public:
		cSoundEntry() : mfNormalVolume(1), mfNormalVolumeFadeDest(1),
						mfNormalVolumeMul(1), mfNormalVolumeFadeSpeed(0), mbStream(false),
						mfBlockRayTime(0), mfFilterGainHF(1){}

		void Update(float afTimeStep);

		tString msName;
		iSoundChannel* mpSound;

		float mfNormalVolume;
		float mfNormalVolumeMul;
		float mfNormalVolumeFadeDest;
		float mfNormalVolumeFadeSpeed;

		float mfNormalSpeed;

		bool mbFirstTime;

		float mfBlockMul;
		float mfBlockFadeDest;
		float mfBlockFadeSpeed;

		bool mbStream;

		// Seconds until the next blocking ray; the first update casts at once.
		float mfBlockRayTime;
		// Low-pass GainHF last sent to the channel; 1 means the filter is off.
		float mfFilterGainHF;

		eSoundDest mEffectType;
	};

	typedef std::list<cSoundEntry> tSoundEntryList;
	typedef tSoundEntryList::iterator tSoundEntryListIt;

	typedef cSTLIterator<cSoundEntry,tSoundEntryList,tSoundEntryListIt> tSoundEntryIterator;


	////////////////////////////////////////////////////
	//////////// SOUND HANDLER ///////////////////////
	////////////////////////////////////////////////////

	//----------------------------------------

	typedef std::map<tString, int> tPlayedSoundNumMap;
	typedef tPlayedSoundNumMap::iterator tPlayedSoundNumMapIt;

	//----------------------------------------

	class cResources;

	//----------------------------------------

	class cSoundHandler
	{
	public:
		cSoundHandler(iLowLevelSound* apLowLevelSound, cResources* apResources);
		~cSoundHandler();

		iSoundChannel* Play(const tString& asName,bool abLoop,float afVolume,const cVector3f& avPos,
				float afMinDist,float afMaxDist, eSoundDest mType, bool abRelative, bool ab3D = false,
				int alPriorityModifier=0,eSoundDest aEffectType=eSoundDest_World);

		iSoundChannel* Play3D(const tString& asName,bool abLoop,float afVolume,const cVector3f& avPos,
			float afMinDist,float afMaxDist, eSoundDest mType, bool abRelative,int alPriorityModifier=0,
			eSoundDest aEffectType=eSoundDest_World)
		{
			return Play(asName,abLoop,afVolume,avPos,afMinDist,afMaxDist,mType,abRelative,true,
						alPriorityModifier, aEffectType);
		}

		iSoundChannel* PlayStream(const tString& asFileName,bool abLoop,float afVolume,bool ab3D = false,
									eSoundDest aEffectType=eSoundDest_Gui);

		iSoundChannel* PlayGui(const tString& asName,bool abLoop,float afVolume,const cVector3f& avPos=cVector3f(0,0,1),
								eSoundDest aEffectType=eSoundDest_Gui);

		void SetSilent(bool abX){ mbSilent = abX; }
		bool GetSilent(){ return mbSilent; }

		bool Stop(const tString& asName);
		bool StopAllExcept(const tString& asName);

		void StopAll(tFlag mTypes);
		void PauseAll(tFlag mTypes);
		void ResumeAll(tFlag mTypes);

		bool IsPlaying(const tString& asName);

		bool IsValid(iSoundChannel* apChannel);
		bool IsValidId(iSoundChannel* apChannel, int alId);

		void Update(float afTimeStep);

		// afRate is the change in speed per second.
		void SetSpeed(float afSpeed,float afRate,tFlag mTypes);
		void SetVolume(float afVolume, float afRate, tFlag mTypes);

		float GetVolume(){ return mfVolume;}

		// Low-pass GainHF (0-1, at 5 kHz) for a fully blocked sound. 1 turns
		// occlusion filtering off; blocked sounds still get quieter.
		void SetBlockedGainHF(float afX);
		float GetBlockedGainHF(){ return mfBlockedGainHF;}

		// Seconds between blocking rays for one sound.
		static constexpr float kBlockRayInterval = 0.5f;

		// Sets the entry's low-pass from its block fade. Called every update;
		// public for the audio tests.
		static void UpdateBlockFilter(cSoundEntry* apEntry, float afBlockedGainHF);

		void SetWorld3D(cWorld3D *apWorld3D);

		cSoundEntry* GetEntryFromSound(iSoundChannel *apSound);

		iSoundChannel* CreateChannel(const tString& asName, int alPriority);

		tSoundEntryList* GetWorldEntryList();
		tSoundEntryList* GetGuiEntryList();
	private:
		iLowLevelSound* mpLowLevelSound;
		cResources* mpResources;

		tSoundEntryList mlstGuiSounds;
		tSoundEntryList mlstWorldSounds;

		bool mbSilent;

		float mfSpeed;
		float mfNewSpeed;
		float mfSpeedRate;
		tFlag mAffectedBySpeed;

		float mfVolume;
		float mfNewVolume;
		float mfVolumeRate;
		tFlag mAffectedByVolume;

		float mfBlockedGainHF;

		cWorld3D *mpWorld3D;

		cSoundRayCallback mSoundRayCallback;

		tPlayedSoundNumMap m_mapPlayedSound;

		cSoundEntry* GetEntry(const tString& asName);
		bool UpdateEntry(cSoundEntry* apEntry,float afTimeStep, tFlag aTypes);
		void UpdateDistanceVolume3D(cSoundEntry* apEntry,float afTimeStep,bool abFade,tFlag aTypes);

		int mlCount;
		int mlIdCount;
	};

};
#endif // HPL_SOUNDHANDLER_H
