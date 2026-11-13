// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_MATERIAL_DIFFUSE_H
#define HPL_MATERIAL_DIFFUSE_H

#include "graphics/Material_BaseLight.h"

namespace hpl {

	class cMaterial_Diffuse : public iMaterial_BaseLight
	{
	public:
		cMaterial_Diffuse(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D);
		~cMaterial_Diffuse() override;

	private:
	};

	class cMaterialType_Diffuse : public iMaterialType
	{
	public:
		cMaterialType_Diffuse();

		bool IsCorrect(tString asName) override{
			return cString::ToLowerCase(asName)=="diffuse";
		}

		iMaterial* Create(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D) override;

	private:
		int mlTechLevel;
	};

};
#endif // HPL_MATERIAL_DIFFUSE_H
