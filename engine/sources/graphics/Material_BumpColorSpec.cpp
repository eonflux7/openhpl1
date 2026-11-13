// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "graphics/Material_BumpColorSpec.h"

#include "graphics/Material_Alpha.h"

namespace hpl {

	cMaterial_BumpColorSpec::cMaterial_BumpColorSpec(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
		cImageManager* apImageManager, cTextureManager *apTextureManager,
		cGpuProgramManager* apProgramManager,
		eMaterialPicture aPicture, cRenderer3D *apRenderer3D)
		: iMaterial_BaseLight("NORMAL_MAP SPECULAR SPECULAR_MAP",
			asName,apLowLevelGraphics,apImageManager,apTextureManager,apProgramManager,
			aPicture,apRenderer3D)
	{
		mbUseSpecular = true;
		mbUseNormalMap = true;
		mbUseColorSpecular = true;
	}

	cMaterial_BumpColorSpec::~cMaterial_BumpColorSpec()
	{

	}

	iMaterial* cMaterialType_BumpColorSpec::Create(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
		cImageManager* apImageManager, cTextureManager *apTextureManager,
		cGpuProgramManager* apProgramManager,
		eMaterialPicture aPicture, cRenderer3D *apRenderer3D)
	{
		return hplNew( cMaterial_BumpColorSpec, (asName,apLowLevelGraphics,
								apImageManager,apTextureManager,
								apProgramManager,aPicture,apRenderer3D) );
	}
}
