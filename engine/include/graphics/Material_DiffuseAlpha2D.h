// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_MATERIAL_DIFFUSE_ALPHA2D_H
#define HPL_MATERIAL_DIFFUSE_ALPHA2D_H

#include "graphics/Material.h"

namespace hpl {

	class cMaterial_DiffuseAlpha2D : public iMaterial
	{
	public:
		cMaterial_DiffuseAlpha2D(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D);
		~cMaterial_DiffuseAlpha2D() override;

		void Compile() override;
		std::optional<cSpriteBlend> GetSpriteBlend() const override { return cSpriteBlend(eBlendFunc_SrcAlpha,eBlendFunc_OneMinusSrcAlpha); }

	private:
	};

	class cMaterialType_DiffuseAlpha2D : public iMaterialType
	{
	public:
		bool IsCorrect(tString asName) override{
			return cString::ToLowerCase(asName)=="diffalpha2d";
		}

		iMaterial* Create(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D) override
		{
			return new cMaterial_DiffuseAlpha2D(asName,apLowLevelGraphics,
				apImageManager,apTextureManager,
				apProgramManager,aPicture,apRenderer3D);
		}
	};

};
#endif // HPL_MATERIAL_DIFFUSE_ALPHA2D_H
