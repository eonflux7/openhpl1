// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "haptic/Haptic.h"
#include "system/LowLevelSystem.h"
#include "haptic/LowLevelHaptic.h"


namespace hpl {

	bool cHaptic::mbIsUsed = false;

	cHaptic::cHaptic(iLowLevelHaptic *apLowLevelHaptic) : iUpdateable("HPL_Haptic")
	{
		mpLowLevelHaptic = apLowLevelHaptic;
	}

	cHaptic::~cHaptic()
	{
		Log("Exiting Haptic Module\n");
		Log("--------------------------------------------------------\n");


		Log("--------------------------------------------------------\n\n");
	}

	void cHaptic::Update(float afTimeStep)
	{
		if(mbIsUsed==false) return;

		mpLowLevelHaptic->Update(afTimeStep);
	}

	void cHaptic::Init(cResources *apResources)
	{
		if(mbIsUsed==false) return;

		Log("Initializing Haptic Module\n");
		Log("--------------------------------------------------------\n");

		if(mpLowLevelHaptic->Init(apResources)==false)
		{
			SetIsUsed(false);
		}

		Log("--------------------------------------------------------\n\n");
	}
}
