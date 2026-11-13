// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "graphics/Material_DiffuseAlpha2D.h"

namespace hpl {

	cMaterial_DiffuseAlpha2D::cMaterial_DiffuseAlpha2D(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
		cImageManager* apImageManager, cTextureManager *apTextureManager,
		cGpuProgramManager* apProgramManager,
		eMaterialPicture aPicture, cRenderer3D *apRenderer3D)
	: iMaterial(asName,apLowLevelGraphics,apImageManager,apTextureManager,apProgramManager,
				aPicture,apRenderer3D)
	{
		mbIsTransparent = true;
		mbIsGlowing= true;//Set to false later on
	}

	cMaterial_DiffuseAlpha2D::~cMaterial_DiffuseAlpha2D()
	{

	}

	void cMaterial_DiffuseAlpha2D::Compile()
	{

	}
}
