// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_GUI_MATERIAL_BASIC_TYPES_H
#define HPL_GUI_MATERIAL_BASIC_TYPES_H

#include "gui/GuiMaterial.h"

namespace hpl {

	class cGuiMaterial_Diffuse : public iGuiMaterial
	{
	public:
		cGuiMaterial_Diffuse(iLowLevelGraphics *apLowLevelGraphics):iGuiMaterial("Diffuse",apLowLevelGraphics){}
		~cGuiMaterial_Diffuse() override{}

		cSpriteBlend GetBlend() const override;
	};

	class cGuiMaterial_Alpha : public iGuiMaterial
	{
	public:
		cGuiMaterial_Alpha(iLowLevelGraphics *apLowLevelGraphics):iGuiMaterial("Alpha",apLowLevelGraphics){}
		~cGuiMaterial_Alpha() override{}

		cSpriteBlend GetBlend() const override;
	};

	class cGuiMaterial_FontNormal : public iGuiMaterial
	{
	public:
		cGuiMaterial_FontNormal(iLowLevelGraphics *apLowLevelGraphics):iGuiMaterial("FontNormal",apLowLevelGraphics){}
		~cGuiMaterial_FontNormal() override{}

		cSpriteBlend GetBlend() const override;
	};

	class cGuiMaterial_Additive : public iGuiMaterial
	{
	public:
		cGuiMaterial_Additive(iLowLevelGraphics *apLowLevelGraphics):iGuiMaterial("Additive",apLowLevelGraphics){}
		~cGuiMaterial_Additive() override{}

		cSpriteBlend GetBlend() const override;
	};

	class cGuiMaterial_Modulative : public iGuiMaterial
	{
	public:
		cGuiMaterial_Modulative(iLowLevelGraphics *apLowLevelGraphics):iGuiMaterial("Modulative",apLowLevelGraphics){}
		~cGuiMaterial_Modulative() override{}

		cSpriteBlend GetBlend() const override;
	};
};
#endif // HPL_GUI_MATERIAL_BASIC_TYPES_H
