// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "graphics/Material_FontNormal.h"

namespace hpl {

	cMaterial_FontNormal::cMaterial_FontNormal(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
		cImageManager* apImageManager, cTextureManager *apTextureManager,
		cGpuProgramManager* apProgramManager,
		eMaterialPicture aPicture, cRenderer3D *apRenderer3D)
	: iMaterial(asName,apLowLevelGraphics,apImageManager,apTextureManager,apProgramManager,
				aPicture,apRenderer3D)
	{
		mbIsTransparent = true;
		mbIsGlowing= true;
	}

	cMaterial_FontNormal::~cMaterial_FontNormal()
	{

	}

	void cMaterial_FontNormal::Compile()
	{

	}
}
