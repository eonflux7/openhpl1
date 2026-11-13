// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_MATERIAL_BUMP_COLOR_SPEC_H
#define HPL_MATERIAL_BUMP_COLOR_SPEC_H

#include "graphics/Material_BaseLight.h"

namespace hpl {

	class cMaterial_BumpColorSpec : public iMaterial_BaseLight
	{
	public:
		cMaterial_BumpColorSpec(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D);
		~cMaterial_BumpColorSpec() override;

	private:
	};

	class cMaterialType_BumpColorSpec : public iMaterialType
	{
	public:
		bool IsCorrect(tString asName) override{
			return cString::ToLowerCase(asName)=="bumpcolorspecular";
		}

		iMaterial* Create(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D) override;
	};

};

#endif // HPL_MATERIAL_BUMP_COLOR_SPEC_H
