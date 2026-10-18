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
#include "input/Action.h"
#include "system/LowLevelSystem.h"

namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	iAction::iAction(tString asName)
	{
		msName = asName;

		mbBecameTriggered= false;
		mbIsTriggered = false;

		mfDoubleTriggerElapsedSeconds = -1.0;

		mbTriggerDown = false;
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	bool iAction::WasTriggered()
	{
		if(mbBecameTriggered && !IsTriggered()){
			mbBecameTriggered=false;
			return true;
		}

		return false;
	}

	//-----------------------------------------------------------------------

	bool iAction::BecameTriggered()
	{
		if(!mbIsTriggered && IsTriggered()){
			mbIsTriggered=true;
			return true;
		}

		return false;
	}

	//-----------------------------------------------------------------------

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

	//-----------------------------------------------------------------------

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

	//-----------------------------------------------------------------------

	void iAction::UpdateLogic(float afTimeStep)
	{
	}

	//-----------------------------------------------------------------------

	tString iAction::GetName()
	{
		return msName;
	}

	//-----------------------------------------------------------------------

}
