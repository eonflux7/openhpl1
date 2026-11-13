// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "input/ActionMouseButton.h"
#include "input/Input.h"
#include "input/Mouse.h"

namespace hpl
{
	cActionMouseButton::cActionMouseButton(tString asName,cInput *apInput, eMButton aButton) : iAction(asName)
	{
		mButton = aButton;
		mpInput = apInput;
	}

	bool cActionMouseButton::IsTriggered()
	{
		return mpInput->GetMouse()->ButtonIsDown(mButton);
	}

	float cActionMouseButton::GetValue()
	{
		if(IsTriggered())return 1.0;
		else return 0.0;
	}

	tString  cActionMouseButton::GetInputName()
	{
		switch(mButton)
		{
			case eMButton_Left: return "LeftMouse";
			case eMButton_Middle: return "MiddleMouse";
			case eMButton_Right: return "RightMouse";
			case eMButton_WheelUp: return "WheelUp";
			case eMButton_WheelDown: return "WheelDown";
			case eMButton_6: return "Mouse6";
			case eMButton_7: return "Mouse7";
			case eMButton_8: return "Mouse8";
			case eMButton_9: return "Mouse9";
		}
		return "Unknown";
	}
}
