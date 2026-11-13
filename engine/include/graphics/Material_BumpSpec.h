// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_MATERIAL_BUMP_SPEC_H
#define HPL_MATERIAL_BUMP_SPEC_H

#include "graphics/Material_BaseLight.h"

namespace hpl {

	class cMaterial_BumpSpec : public iMaterial_BaseLight
	{
	public:
		cMaterial_BumpSpec(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D);
		~cMaterial_BumpSpec() override;

	private:
	};

	class cMaterialType_BumpSpec : public iMaterialType
	{
	public:
		bool IsCorrect(tString asName) override{
			return cString::ToLowerCase(asName)=="bumpspecular";
		}

		iMaterial* Create(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D) override;
	};

};
#endif // HPL_MATERIAL_BUMP_SPEC_H
