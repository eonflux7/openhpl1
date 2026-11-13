// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_ACTIONMOUSEBUTTON_H
#define HPL_ACTIONMOUSEBUTTON_H


#include "input/InputTypes.h"
#include "input/Action.h"

namespace hpl {

	class cInput;

	class cActionMouseButton : public iAction
	{
	public:
		cActionMouseButton(tString asName,cInput* apInput, eMButton aButton);

		bool IsTriggered() override;
		float GetValue() override;

		tString GetInputName() override;

		tString GetInputType() override{return "MouseButton";}

		eMButton GetButton(){ return mButton;}
	private:
		eMButton mButton;
		cInput *mpInput;
	};

};
#endif // HPL_ACTIONMOUSEBUTTON_H
