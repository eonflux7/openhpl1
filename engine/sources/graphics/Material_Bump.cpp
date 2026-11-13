// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "graphics/Material_Bump.h"

#include "graphics/Material_Alpha.h"

namespace hpl {

	cMaterial_Bump::cMaterial_Bump(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
		cImageManager* apImageManager, cTextureManager *apTextureManager,
		cGpuProgramManager* apProgramManager,
		eMaterialPicture aPicture, cRenderer3D *apRenderer3D)
		: iMaterial_BaseLight("NORMAL_MAP",
			asName,apLowLevelGraphics,apImageManager,apTextureManager,apProgramManager,
			aPicture,apRenderer3D)
	{
		mbUseSpecular = false;
		mbUseNormalMap = true;
	}

	cMaterial_Bump::~cMaterial_Bump()
	{

	}

	iMaterial* cMaterialType_Bump::Create(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
						cImageManager* apImageManager, cTextureManager *apTextureManager,
						cGpuProgramManager* apProgramManager,
						eMaterialPicture aPicture, cRenderer3D *apRenderer3D)
	{
		return new cMaterial_Bump(asName,apLowLevelGraphics,
								apImageManager,apTextureManager,
								apProgramManager,aPicture,apRenderer3D);
	}
}
