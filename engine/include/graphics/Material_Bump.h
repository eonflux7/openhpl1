// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_MATERIAL_BUMP_H
#define HPL_MATERIAL_BUMP_H

#include "graphics/Material_BaseLight.h"

namespace hpl {

	class cMaterial_Bump : public iMaterial_BaseLight
	{
	public:
		cMaterial_Bump(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D);
		~cMaterial_Bump() override;

	private:
	};

	class cMaterialType_Bump : public iMaterialType
	{
	public:
		bool IsCorrect(tString asName) override{
			return cString::ToLowerCase(asName)=="bump";
		}

		iMaterial* Create(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D) override;
	};

};
#endif // HPL_MATERIAL_BUMPE_H
