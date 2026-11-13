// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_GUI_POP_UP_H
#define HPL_GUI_POP_UP_H

#include "gui/GuiTypes.h"

namespace hpl {

	class cGuiSet;
	class cGuiSkin;

	class iGuiPopUp
	{
	public:
		iGuiPopUp(cGuiSet *apSet);
		virtual ~iGuiPopUp();

	protected:
		void SelfDestruct();

		cGuiSet *mpSet;
		cGuiSkin *mpSkin;
	};

};
#endif // HPL_WIDGET_FRAME_H
