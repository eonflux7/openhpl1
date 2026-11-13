// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "input/Action.h"
#include "system/LowLevelSystem.h"

namespace hpl {

	iAction::iAction(tString asName)
	{
		msName = asName;

		mbBecameTriggered= false;
		mbIsTriggered = false;

		mfDoubleTriggerElapsedSeconds = -1.0;

		mbTriggerDown = false;
	}

	bool iAction::WasTriggered()
	{
		if(mbBecameTriggered && !IsTriggered()){
			mbBecameTriggered=false;
			return true;
		}

		return false;
	}

	bool iAction::BecameTriggered()
	{
		if(!mbIsTriggered && IsTriggered()){
			mbIsTriggered=true;
			return true;
		}

		return false;
	}

	bool iAction::DoubleTriggered(float afMaxIntervalSeconds)
	{
		if(!mbTriggerDown && IsTriggered())
		{
			mbTriggerDown=true;

			if(mfDoubleTriggerElapsedSeconds <0 || mfDoubleTriggerElapsedSeconds > afMaxIntervalSeconds)
			{
				mfDoubleTriggerElapsedSeconds =0;
				return false;
			}
			else
			{
				mfDoubleTriggerElapsedSeconds =0;
				mbIsTriggered=true;
				return true;
			}
		}

		return false;
	}

	void iAction::Update(float afTimeStep)
	{
		UpdateLogic(afTimeStep);

		if(!IsTriggered()){
			mbIsTriggered=false;
			mbTriggerDown = false;

			if(mfDoubleTriggerElapsedSeconds >= 0) mfDoubleTriggerElapsedSeconds += afTimeStep;
		}
		else {
			mbBecameTriggered=true;
		}
	}

	void iAction::UpdateLogic(float afTimeStep)
	{
	}

	tString iAction::GetName()
	{
		return msName;
	}
}
