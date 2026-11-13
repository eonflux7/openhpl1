// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_GUI_MATERIAL_H
#define HPL_GUI_MATERIAL_H

#include "system/SystemTypes.h"
#include "graphics/SpriteBatch.h"
namespace hpl {

	class iLowLevelGraphics;

	class iGuiMaterial
	{
	public:
		iGuiMaterial(const tString &asName,iLowLevelGraphics *apLowLevelGraphics) :
						msName(asName), mpLowLevelGraphics(apLowLevelGraphics) {}
		virtual ~iGuiMaterial(){}

		virtual cSpriteBlend GetBlend() const=0;

		const tString& GetName() const {return msName;}

	protected:
		tString msName;
		iLowLevelGraphics *mpLowLevelGraphics;
	};

};
#endif // HPL_GUI_MATERIAL_H
