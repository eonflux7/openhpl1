// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "input/ActionHaptic.h"
#include "input/Input.h"
#include "haptic/Haptic.h"
#include "haptic/LowLevelHaptic.h"

namespace hpl
{
	cActionHaptic::cActionHaptic(tString asName,cHaptic *apHaptic, int alButton) : iAction(asName)
	{
		mlButton = alButton;
		mpHaptic = apHaptic;
	}

	bool cActionHaptic::IsTriggered()
	{
		if(mlButton >= mpHaptic->GetLowLevel()->GetNumberOfButtons()) return false;

		return mpHaptic->GetLowLevel()->ButtonIsPressed(mlButton);
	}

	float cActionHaptic::GetValue()
	{
		if(IsTriggered())return 1.0;
		else return 0.0;
	}

	tString cActionHaptic::GetInputName()
	{
		switch(mlButton)
		{
		case 0: return "centre Controller button";
		case 1: return "left Controller button";
		case 2: return "forward Controller button";
		case 3: return "right Controller button";
		}
		return "unknown Controller button";
	}
}
