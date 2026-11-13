// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "system/LogicTimer.h"
#include "system/LowLevelSystem.h"

namespace hpl {

	cLogicTimer::cLogicTimer(int alUpdatesPerSec, iLowLevelSystem *apLowLevelSystem)
	{
		mlMaxUpdates = alUpdatesPerSec;
		mlUpdateCount =0;
		mbFixedStep = false;
		mbPaceFixedStep = false;
		mlFixedStepsPerFrame = 1;

		mpLowLevelSystem = apLowLevelSystem;

		SetUpdatesPerSec(alUpdatesPerSec);
	}

	cLogicTimer::~cLogicTimer()
	{
	}

	void cLogicTimer::Reset()
	{
		mfNextUpdateTimeMs = GetApplicationTimeExact();
	}

	bool cLogicTimer::WantUpdate()
	{
		++mlUpdateCount;
		if(mbFixedStep)
		{
			if(mlUpdateCount > mlFixedStepsPerFrame) return false;
			if(mbPaceFixedStep)
			{
				double fNow = GetApplicationTimeExact();
				if(mfNextUpdateTimeMs > fNow)
					mpLowLevelSystem->SleepPrecise(mfNextUpdateTimeMs - fNow);
				mfNextUpdateTimeMs += mfStepMilliseconds;
				// After a long frame (a map load) do not run to catch up
				fNow = GetApplicationTimeExact();
				if(mfNextUpdateTimeMs < fNow) mfNextUpdateTimeMs = fNow;
			}
			return true;
		}
		if(mlUpdateCount > mlMaxUpdates) return false;

		if(mfNextUpdateTimeMs< GetApplicationTimeExact())
		{
			Update();
			return true;
		}
		return false;
	}

	double cLogicTimer::GetMillisecsToUpdate()
	{
		if(mbFixedStep) return 0;
		double fMs = mfNextUpdateTimeMs - GetApplicationTimeExact();
		return fMs > 0 ? fMs : 0;
	}

	void cLogicTimer::EndUpdateLoop()
	{
		if(mlUpdateCount > mlMaxUpdates){
			Reset();
		}

		mlUpdateCount=0;
	}

	void cLogicTimer::SetUpdatesPerSec(int alUpdatesPerSec)
	{
		mfStepMilliseconds = 1000.0/((double)alUpdatesPerSec);
		Reset();
	}

	void cLogicTimer::SetMaxUpdates(int alMaxUpdates)
	{
		mlMaxUpdates = alMaxUpdates;
	}

	int cLogicTimer::GetUpdatesPerSec()
	{
		return (int)(1000.0 / ((double)mfStepMilliseconds));
	}

	float cLogicTimer::GetStepSize()
	{
		return ((float)mfStepMilliseconds)/1000.0f;
	}

	float cLogicTimer::GetInterpolation()
	{
		if(mbFixedStep) return 1.0f;

		// mfNextUpdateTimeMs is when the next update is due.
		double fT = 1.0 - (mfNextUpdateTimeMs - GetApplicationTimeExact()) / mfStepMilliseconds;
		if(fT < 0.0) fT = 0.0;
		if(fT > 1.0) fT = 1.0;
		return (float)fT;
	}

	void cLogicTimer::Update()
	{
		mfNextUpdateTimeMs+=mfStepMilliseconds;
	}
}
