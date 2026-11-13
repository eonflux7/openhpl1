// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_HAPTIC_FORCE_H
#define HPL_HAPTIC_FORCE_H

#include "math/MathTypes.h"
#include "haptic/HapticTypes.h"

namespace hpl {

	class iHapticForce
	{
	public:
		iHapticForce(eHapticForceType aType) : mType(aType){}
		virtual ~iHapticForce(){}

		eHapticForceType GetType(){ return mType;}

		virtual void SetActive(bool abX)=0;
		virtual bool IsActive()=0;

		virtual void SetForce(const cVector3f &avForce)=0;
		virtual void SetRelativeForce(const cVector3f &avForce)=0;

		virtual void SetDirection(const cVector3f &avDir)=0;
		virtual void SetFreq(float afFreq)=0;
		virtual void SetAmp(float afAmp)=0;

		virtual void SetTimeControl(bool abLoop,float afTime,float afIdleTime,
									float afFadeInTime, float afFadeOutTime)=0;

	protected:
		eHapticForceType mType;
	};

};
#endif // HPL_HAPTIC_FORCE_H