// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_ACTION_HAPTIC_H
#define HPL_ACTION_HAPTIC_H


#include "input/InputTypes.h"
#include "input/Action.h"
#include "haptic/Haptic.h"

namespace hpl {

	class cHaptic;

	class cActionHaptic : public iAction
	{
	public:
		cActionHaptic(tString asName,cHaptic *apHaptic, int alButton);

		bool IsTriggered() override;
		float GetValue() override;

		tString GetInputName() override;

		tString GetInputType() override{return "HapticDeviceButton";}

		int GetButton(){ return mlButton;}
	private:
		int mlButton;
		cHaptic *mpHaptic;
	};

};
#endif // HPL_ACTION_HAPTIC_H
