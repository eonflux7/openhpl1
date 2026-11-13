// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "gui/GuiPopUp.h"

#include "system/LowLevelSystem.h"

#include "math/Math.h"

#include "gui/Gui.h"
#include "gui/GuiSkin.h"
#include "gui/GuiSet.h"

namespace hpl {

	iGuiPopUp::iGuiPopUp(cGuiSet *apSet)
	{
		mpSet = apSet;
		mpSkin = mpSet->GetSkin();
	}

	iGuiPopUp::~iGuiPopUp()
	{
	}

	void iGuiPopUp::SelfDestruct()
	{
		mpSet->DestroyPopUp(this);
	}
}
