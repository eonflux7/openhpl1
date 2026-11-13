// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_MATERIAL_DIFFUSE_ADDITIVE2D_H
#define HPL_MATERIAL_DIFFUSE_ADDITIVE2D_H

#include "graphics/Material.h"

namespace hpl {

	class cMaterial_DiffuseAdditive2D : public iMaterial
	{
	public:
		cMaterial_DiffuseAdditive2D(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D);
		~cMaterial_DiffuseAdditive2D() override;

		void Compile() override;
		std::optional<cSpriteBlend> GetSpriteBlend() const override { return cSpriteBlend(eBlendFunc_SrcAlpha,eBlendFunc_One); }

		bool UsesType(eMaterialRenderType aType) override;

	private:
	};

	class cMaterialType_DiffuseAdditive2D : public iMaterialType
	{
	public:
		bool IsCorrect(tString asName) override{
			if(cString::ToLowerCase(asName)=="diffadditive2d")return true;
			return false;
		}

		iMaterial* Create(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D) override
		{
			return hplNew( cMaterial_DiffuseAdditive2D, (asName,apLowLevelGraphics,
				apImageManager,apTextureManager,
				apProgramManager,aPicture,apRenderer3D) );
		}
	};

};
#endif // HPL_MATERIAL_DIFFUSE_ADDITIVE_H
