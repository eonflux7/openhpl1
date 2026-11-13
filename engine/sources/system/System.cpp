// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "system/System.h"
#include "system/LowLevelSystem.h"
#include "system/LogicTimer.h"
#include "system/String.h"

namespace hpl {

	cSystem::cSystem(iLowLevelSystem *apLowLevelSystem)
	{
		mpLowLevelSystem = apLowLevelSystem;
	}

	cSystem::~cSystem()
	{
		Log("Exiting System Module\n");
		Log("--------------------------------------------------------\n");

		Log("--------------------------------------------------------\n\n");
	}

	cLogicTimer * cSystem::CreateLogicTimer(unsigned int alUpdatesPerSec)
	{
		return new cLogicTimer(alUpdatesPerSec, mpLowLevelSystem);
	}

	iLowLevelSystem* cSystem::GetLowLevel()
	{
		return mpLowLevelSystem;
	}
}
