// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_MATERIAL_WATER_H
#define HPL_MATERIAL_WATER_H

#include "graphics/Material.h"

namespace hpl {

	class cMaterial_Water : public iMaterial
	{
	public:
		cMaterial_Water(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D);

		~cMaterial_Water() override;

		tTextureTypeList GetTextureTypes() override;

		bool UsesType(eMaterialRenderType aType) override;

		void Update(float afTimeStep) override;

		iGpuProgram* GetProgram(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		iMaterialProgramSetup* GetProgramSetup(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		bool ProgramUsesLight(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		bool ProgramUsesEye(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;

		eMaterialAlphaMode GetAlphaMode(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		eMaterialBlendMode GetBlendMode(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		eMaterialChannelMode GetChannelMode(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;

		iTexture* GetTexture(int alUnit,eMaterialRenderType aType, int alPass, iLight3D *apLight) override;

		int GetNumOfPasses(eMaterialRenderType aType, iLight3D *apLight) override{ return 1;}

		iGpuProgram* GetRefractionProgram() override{ return mpRefractProgram;}
		bool GetRefractionUsesDiffuse() override{ return true;}
		eMaterialTexture GetRefractionDiffuseTexture() override{ return eMaterialTexture_Specular;}
		bool GetRefractionUsesEye() override{ return true;}
		bool GetRefractionSkipsStandardTrans() override{ return true;}
		bool GetRefractionUsesTime() override{ return true;}

		void Compile() override{}

	private:
		iGpuProgram *mpFogProgram;

		iGpuProgram *mpRefractProgram;

		float mfTime;
	};

	class cMaterialType_Water : public iMaterialType
	{
	public:
		bool IsCorrect(tString asName) override{
			return cString::ToLowerCase(asName)=="water";
		}

		iMaterial* Create(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D) override
		{
			return hplNew( cMaterial_Water, (asName,apLowLevelGraphics,
				apImageManager,apTextureManager,
				apProgramManager,aPicture,apRenderer3D) );
		}
	};

};
#endif // HPL_MATERIAL_WATER_H
