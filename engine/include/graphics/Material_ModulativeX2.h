// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_MATERIAL_MODULATIVE_X_2_H
#define HPL_MATERIAL_MODULATIVE_X_2_H

#include "graphics/Material.h"

namespace hpl {

	class cMaterial_ModulativeX2 : public iMaterial
	{
	public:
		cMaterial_ModulativeX2(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D);

		~cMaterial_ModulativeX2() override;

		tTextureTypeList GetTextureTypes() override;

		bool UsesType(eMaterialRenderType aType) override;

		iGpuProgram* GetProgram(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		iMaterialProgramSetup* GetProgramSetup(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		bool ProgramUsesLight(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		bool ProgramUsesEye(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;

		eMaterialAlphaMode GetAlphaMode(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		eMaterialBlendMode GetBlendMode(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		eMaterialChannelMode GetChannelMode(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;

		iTexture* GetTexture(int alUnit,eMaterialRenderType aType, int alPass, iLight3D *apLight) override;

		int GetNumOfPasses(eMaterialRenderType aType, iLight3D *apLight) override{ return 1;}

		void Compile() override{}

	private:
		iGpuProgram *mpFogProgram;
	};

	class cMaterialType_ModulativeX2 : public iMaterialType
	{
	public:
		bool IsCorrect(tString asName) override{
			return cString::ToLowerCase(asName)=="modulativex2";
		}

		iMaterial* Create(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D) override
		{
			return new cMaterial_ModulativeX2(asName,apLowLevelGraphics,
				apImageManager,apTextureManager,
				apProgramManager,aPicture,apRenderer3D);
		}
	};

};
#endif // HPL_MATERIAL_MODULATIVE_X_2_H
