// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "gui/GuiMaterialBasicTypes.h"

#include "graphics/LowLevelGraphics.h"

namespace hpl {
	cSpriteBlend cGuiMaterial_Diffuse::GetBlend() const
	{
		return cSpriteBlend(eBlendFunc_One,eBlendFunc_Zero);
	}

	cSpriteBlend cGuiMaterial_Alpha::GetBlend() const
	{
		return cSpriteBlend(eBlendFunc_SrcAlpha,eBlendFunc_OneMinusSrcAlpha);
	}

	cSpriteBlend cGuiMaterial_FontNormal::GetBlend() const
	{
		return cSpriteBlend(eBlendFunc_SrcAlpha,eBlendFunc_OneMinusSrcAlpha);
	}

	cSpriteBlend cGuiMaterial_Additive::GetBlend() const
	{
		return cSpriteBlend(eBlendFunc_One,eBlendFunc_One);
	}

	cSpriteBlend cGuiMaterial_Modulative::GetBlend() const
	{
		return cSpriteBlend(eBlendFunc_DestColor,eBlendFunc_Zero);
	}

}
