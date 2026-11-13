// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_MATERIAL_TRANSLUCENT_H
#define HPL_MATERIAL_TRANSLUCENT_H

#include "graphics/Material.h"

namespace hpl {

	// The unlit see-through materials, Additive, Alpha, Modulative and
	// ModulativeX2: a diffuse texture blended onto the scene, with the fog of
	// the matching blend mode when fog is on.
	struct cTranslucentMaterialMode
	{
		const char *msName;
		eMaterialBlendMode mBlendMode;
		const char *msFogDefines;
		bool mbAlphaFog;
	};

	class cMaterial_Translucent : public iMaterial
	{
	public:
		cMaterial_Translucent(const cTranslucentMaterialMode &aMode, const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D);

		~cMaterial_Translucent() override;

		tTextureTypeList GetTextureTypes() override;

		bool UsesType(eMaterialRenderType aType) override{ return aType == eMaterialRenderType_Diffuse; }

		iGpuProgram* GetProgram(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		iMaterialProgramSetup* GetProgramSetup(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;

		bool ProgramUsesLight(eMaterialRenderType aType, int alPass, iLight3D *apLight) override{ return false; }
		bool ProgramUsesEye(eMaterialRenderType aType, int alPass, iLight3D *apLight) override{ return false; }

		eMaterialAlphaMode GetAlphaMode(eMaterialRenderType aType, int alPass, iLight3D *apLight) override{ return eMaterialAlphaMode_Solid; }
		eMaterialBlendMode GetBlendMode(eMaterialRenderType aType, int alPass, iLight3D *apLight) override{ return mMode.mBlendMode; }
		eMaterialChannelMode GetChannelMode(eMaterialRenderType aType, int alPass, iLight3D *apLight) override{ return eMaterialChannelMode_RGBA; }

		iTexture* GetTexture(int alUnit,eMaterialRenderType aType, int alPass, iLight3D *apLight) override;

		int GetNumOfPasses(eMaterialRenderType aType, iLight3D *apLight) override{ return 1;}

		void Compile() override{}

	private:
		cTranslucentMaterialMode mMode;
		iGpuProgram *mpFogProgram;
	};

	class cMaterialType_Translucent : public iMaterialType
	{
	public:
		cMaterialType_Translucent(const cTranslucentMaterialMode &aMode) : mMode(aMode){}

		bool IsCorrect(tString asName) override{
			return cString::ToLowerCase(asName)==mMode.msName;
		}

		iMaterial* Create(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D) override
		{
			return new cMaterial_Translucent(mMode,asName,apLowLevelGraphics,
				apImageManager,apTextureManager,
				apProgramManager,aPicture,apRenderer3D);
		}

	private:
		cTranslucentMaterialMode mMode;
	};

};
#endif // HPL_MATERIAL_TRANSLUCENT_H
