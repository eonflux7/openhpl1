// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "resources/ResourceBase.h"

#include "system/LowLevelSystem.h"


namespace hpl {

	bool iResourceBase::mbLogCreateAndDelete=false;

	iResourceBase::iResourceBase(tString asName,unsigned long alPrio){
		mlTime = (unsigned long)time(nullptr);
		mlPrio = alPrio;
		mlHandle = 0;
		mlUserCount =0;
		msName = asName;
		mbLogDestruction = false;
	}

	iResourceBase::~iResourceBase()
	{
		if(mbLogDestruction && mbLogCreateAndDelete)
			Log("  Destroyed resource '%s'\n",msName.c_str());
	}

	void iResourceBase::IncUserCount()
	{
		mlUserCount++;
		mlTime = (unsigned long)time(nullptr);
	}
}
