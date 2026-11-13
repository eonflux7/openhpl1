// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_SOUND_ENTITY_H
#define HPL_SOUND_ENTITY_H

#include "scene/Entity3D.h"
#include "sound/SoundChannel.h"

namespace hpl {

	class cSoundHandler;
	class cSoundEntityData;
	class cSoundEntityManager;
	class cWorld3D;
	class cSoundEntry;

	enum eSoundEntityType
	{
		eSoundEntityType_Main,
		eSoundEntityType_Start,
		eSoundEntityType_Stop,
		eSoundEntityType_LastEnum
	};

	class cSoundEntity;

	class cSoundEntityChannelCallback : public iSoundChannelCallback
	{
	public:
		void OnPriorityRelease() override;

		cSoundEntity *mpEntity;
	};

	kSaveData_ChildClass(iEntity3D, cSoundEntity)
	{
		kSaveData_ClassInit(cSoundEntity)
	public:
		tString msData;
		bool mbStopped;
		bool mbRemoveWhenOver;
		bool mbStarted;
		bool mbFadingOut;
		float mfVolume;

		iSaveObject* CreateSaveObject(cSaveObjectHandler *apSaveObjectHandler,cGame *apGame) override;
		int GetSaveCreatePrio() override;
	};

	class iSoundEntityGlobalCallback
	{
	public:
		virtual ~iSoundEntityGlobalCallback(){}
		virtual void OnStart(cSoundEntity *apSoundEntity)=0;
	};

	typedef std::list<iSoundEntityGlobalCallback*> tSoundEntityGlobalCallbackList;
	typedef tSoundEntityGlobalCallbackList::iterator tSoundEntityGlobalCallbackListIt;

	class cSoundEntity : public iEntity3D
	{
	#ifdef __GNUC__
		typedef iEntity3D __super;
	#endif
	friend class cSoundEntityChannelCallback;
	public:
		cSoundEntity(const tString& asName,cSoundEntityData *apData,
						cSoundEntityManager *apSoundEntityManager,
						cWorld3D *apWorld,
						cSoundHandler *apSoundHandler, bool abRemoveWhenOver);
		~cSoundEntity() override;

		void Play(bool abPlayStart=true);
		void Stop(bool abPlayEnd=true);

		void FadeIn(float afSpeed);
		void FadeOut(float afSpeed);

		bool IsStopped();
		bool IsFadingOut();
		bool GetRemoveWhenOver();

		void SetVolume(float afX){ mfVolume = afX;}
		float GetVolume(){ return mfVolume;}

		iSoundChannel* GetSound(eSoundEntityType aType){ return mvSounds[aType];}
		cSoundEntry* GetSoundEntry(eSoundEntityType aType);

		//Entity implementation
		void UpdateLogic(float afTimeStep) override;

		tString GetEntityType() override{ return "SoundEntity";}

		cSoundEntityData* GetData(){ return mpData;}

		static void AddGlobalCallback(iSoundEntityGlobalCallback *apCallback);
		static void RemoveGlobalCallback(iSoundEntityGlobalCallback *apCallback);


		//SaveObject implementation
		iSaveData* CreateSaveData() override;
		void SaveToSaveData(iSaveData *apSaveData) override;
		void LoadFromSaveData(iSaveData *apSaveData) override;
		void SaveDataSetup(cSaveObjectHandler *apSaveObjectHandler, cGame *apGame) override;

	private:
		bool PlaySound(const tString &asName,bool abLoop,eSoundEntityType aType);

		float GetListenerSqrLength();

		cSoundEntityManager *mpSoundEntityManager;
		cSoundHandler *mpSoundHandler;
		cSoundEntityData *mpData;
		cWorld3D *mpWorld;

		iSoundChannel *mvSounds[3];
		int mvSoundId[3];

		bool mbStopped;
		bool mbRemoveWhenOver;

		bool mbOutOfRange;

		float mfIntervalCount;

		cSoundEntityChannelCallback *mpSoundCallback;

		bool mbStarted;

		bool mbFadingOut;

		bool mbLog;

		float mfVolume;

		bool mbPrioRemove;

		bool mbSkipStartEnd;

		float mfSleepCount;

		static tSoundEntityGlobalCallbackList mlstGobalCallbacks;
	};

};
#endif // HPL_SOUND_ENTITY_H
