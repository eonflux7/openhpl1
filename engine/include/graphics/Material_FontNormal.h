// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_MATERIAL_FONTNORMAL_H
#define HPL_MATERIAL_FONTNORMAL_H

#include "graphics/Material.h"

namespace hpl {

	class cMaterial_FontNormal : public iMaterial
	{
	public:
		cMaterial_FontNormal(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D);
		~cMaterial_FontNormal() override;

		void Compile() override;
		std::optional<cSpriteBlend> GetSpriteBlend() const override { return cSpriteBlend(eBlendFunc_SrcAlpha,eBlendFunc_OneMinusSrcAlpha); }

	private:
	};

	class cMaterialType_FontNormal : public iMaterialType
	{
	public:
		bool IsCorrect(tString asName) override{
			return cString::ToLowerCase(asName)=="fontnormal";
		}

		iMaterial* Create(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D) override
		{
			return new cMaterial_FontNormal(asName,apLowLevelGraphics,
				apImageManager,apTextureManager,
				apProgramManager,aPicture,apRenderer3D);
		}
	};

};
#endif // HPL_MATERIAL_FONTNORMAL_H
