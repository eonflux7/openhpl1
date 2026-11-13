// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "graphics/Material_DiffuseAdditive2D.h"

namespace hpl {

	cMaterial_DiffuseAdditive2D::cMaterial_DiffuseAdditive2D(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
		cImageManager* apImageManager, cTextureManager *apTextureManager,
		cGpuProgramManager* apProgramManager,
		eMaterialPicture aPicture, cRenderer3D *apRenderer3D)
	: iMaterial(asName,apLowLevelGraphics,apImageManager,apTextureManager,apProgramManager,
				aPicture,apRenderer3D)
	{
		mbIsTransparent = true;
		mbIsGlowing= true;
	}

	cMaterial_DiffuseAdditive2D::~cMaterial_DiffuseAdditive2D()
	{

	}

	void cMaterial_DiffuseAdditive2D::Compile()
	{

	}

	bool cMaterial_DiffuseAdditive2D::UsesType(eMaterialRenderType aType)
	{
		if(aType == eMaterialRenderType_Diffuse)return true;

		return false;
	}
}
