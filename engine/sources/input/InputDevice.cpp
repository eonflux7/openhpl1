// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "input/InputDevice.h"

namespace hpl
{
	iInputDevice::iInputDevice(tString asName,eInputDeviceType aType)
	: msName(asName), mType(aType)
	{

	}

	tString iInputDevice::GetName(){
		return msName;
	}

	eInputDeviceType iInputDevice::GetType(){
		return mType;
	}
}
