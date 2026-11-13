// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_ANIMATION_STATE_H
#define HPL_ANIMATION_STATE_H

#include "math/MathTypes.h"
#include "system/SystemTypes.h"
#include "graphics/GraphicsTypes.h"

#include "game/SaveGame.h"

namespace hpl {

	class cAnimation;
	class cAnimationManager;

	class cSaveData_cAnimationState : public iSaveData
	{
		kSaveData_ClassInit(cAnimationState)
	public:
		tString msName;

		tString msAnimationName;
		float mfDefaultSpeed = 1; // Retained for older generic engine saves.

		float mfLength;
		float mfWeight;
		float mfSpeed;
		float mfTimePos;

		float mfBaseSpeed = 1;

		bool mbActive;
		bool mbLoop;
		bool mbPaused;

		float mfFadeStep;

		float mfSpecialEventTime;

		iSaveObject* CreateSaveObject(cSaveObjectHandler *apSaveObjectHandler,cGame *apGame) override;
		int GetSaveCreatePrio() override;
	};

	class cAnimationEvent
	{
	public:
		float mfTime;
		eAnimationEventType mType;
		tString msValue;
	};

	class cAnimationState : public iSaveObject
	{
	#ifdef __GNUC__
		typedef iSaveObject __super;
	#endif
	public:
		cAnimationState(cAnimation* apAnimation,const tString &asName,
						cAnimationManager *apAnimationManager);
		~cAnimationState() override;

		const char* GetName(){ return msName.c_str();}

		void Update(float afTimeStep);

		bool IsFading();

		/**
		 * If the animation has reached the end.
		 */
		bool IsOver();

		void FadeIn(float afTime);
		void FadeOut(float afTime);

		void SetLength(float afLength);
		float GetLength();

		/**
		 * True if the time was set, or the state started or stopped, from
		 * outside Update() since this was last asked; asking clears it. The
		 * pose may then jump, and a mesh does not blend into it.
		 */
		bool TakeTimeJump(){ bool bX = mbTimeJumped; mbTimeJumped = false; return bX; }

		void SetWeight(float afWeight);
		float GetWeight();

		void SetSpeed(float afSpeed);
		float GetSpeed();

		void SetBaseSpeed(float afSpeed);
		float GetBaseSpeed();

		void SetTimePosition(float afPosition);
		float GetTimePosition();
		float GetPreviousTimePosition();
		// Events crossed by the last update, including looping and reverse playback.
		bool HasEventInLastUpdate(float afEventTime) const;

		/**
		 * Set the relative position. 0 = start, 1 = end
		 * \param afPosition
		 */
		void SetRelativeTimePosition(float afPosition);

		/**
		* Get the relative position. 0 = start, 1 = end
		*/
		float GetRelativeTimePosition();

		bool IsActive();
		void SetActive(bool abActive);

		bool IsLooping();
		void SetLoop(bool abLoop);

		bool IsPaused();
		void SetPaused(bool abPaused);

		void SetSpecialEventTime(float afT){ mfSpecialEventTime = afT;}
		float GetSpecialEventTime(){ return mfSpecialEventTime;}
		bool IsAfterSpecialEvent();
		bool IsBeforeSpecialEvent();

		void AddTimePosition(float afAdd);

		cAnimation* GetAnimation();

		cAnimationEvent *CreateEvent();
		cAnimationEvent *GetEvent(int alIdx);
		int GetEventNum();

		float GetFadeStep(){ return mfFadeStep;}
		void SetFadeStep(float afX){ mfFadeStep = afX;}

		//SaveObject implementation
		iSaveData* CreateSaveData() override;
		void SaveToSaveData(iSaveData *apSaveData) override;
		void LoadFromSaveData(iSaveData *apSaveData) override;
		void SaveDataSetup(cSaveObjectHandler *apSaveObjectHandler, cGame *apGame) override;

	private:
		tString msName;

		cAnimationManager *mpAnimationManager;

		cAnimation* mpAnimation;

		std::vector<cAnimationEvent*> mvEvents;

		//Properties of the animation
		float mfLength;
		float mfWeight;
		float mfSpeed;
		float mfTimePos;
		float mfPrevTimePos;
		float mfTimeDelta;

		float mfBaseSpeed;

		float mfSpecialEventTime;

		bool mbActive;
		bool mbLoop;
		bool mbPaused;

		bool mbTimeJumped;

		//properties for update
		float mfFadeStep;
	};

};
#endif // HPL_ANIMATION_STATE_H
