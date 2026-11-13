// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "graphics/Material_DiffuseSpec.h"


namespace hpl {

	cMaterial_DiffuseSpec::cMaterial_DiffuseSpec(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
		cImageManager* apImageManager, cTextureManager *apTextureManager,
		cGpuProgramManager* apProgramManager,
		eMaterialPicture aPicture, cRenderer3D *apRenderer3D)
		: iMaterial_BaseLight("SPECULAR",
			asName,apLowLevelGraphics,apImageManager,apTextureManager,apProgramManager,
			aPicture,apRenderer3D)
	{
		mbUseSpecular = true;
		mbUseNormalMap = false;
	}

	cMaterial_DiffuseSpec::~cMaterial_DiffuseSpec()
	{

	}

	iMaterial* cMaterialType_DiffuseSpec::Create(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
		cImageManager* apImageManager, cTextureManager *apTextureManager,
		cGpuProgramManager* apProgramManager,
		eMaterialPicture aPicture, cRenderer3D *apRenderer3D)
	{
		return new cMaterial_DiffuseSpec(asName,apLowLevelGraphics,
								apImageManager,apTextureManager,
								apProgramManager,aPicture,apRenderer3D);
	}
}
