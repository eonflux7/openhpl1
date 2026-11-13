// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_ACTIONKEYBOARD_H
#define HPL_ACTIONKEYBOARD_H


#include "input/InputTypes.h"
#include "input/Action.h"

namespace hpl {

	class cInput;

	class cActionKeyboard : public iAction
	{
	public:
		cActionKeyboard(tString asName,cInput* apInput, int aKey);

		bool IsTriggered() override;
		float GetValue() override;

		tString GetInputName() override;

		tString GetInputType() override{return "Keyboard";}

		eKey GetKey(){ return mKey;}
		eKeyModifier GetModifier(){ return mMod;}

	private:
		eKey mKey;
		eKeyModifier mMod;
		cInput *mpInput;
	};

};
#endif // HPL_ACTIONKEYBOARD_H
