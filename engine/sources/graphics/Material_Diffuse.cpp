// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "graphics/Material_Diffuse.h"


namespace hpl {

	cMaterial_Diffuse::cMaterial_Diffuse(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
		cImageManager* apImageManager, cTextureManager *apTextureManager,
		cGpuProgramManager* apProgramManager,
		eMaterialPicture aPicture, cRenderer3D *apRenderer3D)
		: iMaterial_BaseLight("",
			asName,apLowLevelGraphics,apImageManager,apTextureManager,apProgramManager,
			aPicture,apRenderer3D)
	{
		mbUseSpecular = false;
		mbUseNormalMap = false;
	}

	cMaterial_Diffuse::~cMaterial_Diffuse()
	{

	}

	cMaterialType_Diffuse::cMaterialType_Diffuse()
	{
		mlTechLevel =0;
	}

	iMaterial* cMaterialType_Diffuse::Create(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
										cImageManager* apImageManager, cTextureManager *apTextureManager,
										cGpuProgramManager* apProgramManager,
										eMaterialPicture aPicture, cRenderer3D *apRenderer3D)
	{
		return hplNew( cMaterial_Diffuse, (asName,apLowLevelGraphics,
								apImageManager,apTextureManager,
								apProgramManager,aPicture,apRenderer3D) );
	}
}
