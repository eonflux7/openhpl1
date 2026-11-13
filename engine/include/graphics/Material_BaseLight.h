// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_MATERIAL_BASE_LIGHT_H
#define HPL_MATERIAL_BASE_LIGHT_H

#include <vector>
#include "graphics/Material.h"
#include "scene/Light3D.h"

enum eBaseLightProgram
{
	eBaseLightProgram_Point1=0,
	eBaseLightProgram_Point2=1,
	eBaseLightProgram_Point3=2,
	eBaseLightProgram_Spot1=3,
	eBaseLightProgram_Spot2=4,
	eBaseLightProgram_Spot3=5,
	eBaseLightProgram_LastEnum = 6
};

namespace hpl {

	class iMaterial_BaseLight : public iMaterial
	{
	public:
		iMaterial_BaseLight(const tString& asLightDefines,

			const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D);

		~iMaterial_BaseLight() override;

		tTextureTypeList GetTextureTypes() override;

		bool UsesType(eMaterialRenderType aType) override;

		iGpuProgram* GetProgram(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		bool ProgramUsesLight(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		bool ProgramUsesEye(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		iMaterialProgramSetup * GetProgramSetup(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;

		eMaterialAlphaMode GetAlphaMode(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		eMaterialBlendMode GetBlendMode(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		eMaterialChannelMode GetChannelMode(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;

		iTexture* GetTexture(int alUnit,eMaterialRenderType aType, int alPass, iLight3D *apLight) override;

		int GetNumOfPasses(eMaterialRenderType aType, iLight3D *apLight) override;

		void Compile() override{}

	protected:
		//properties to set
		bool mbUseSpecular;
		bool mbUseNormalMap;
		bool mbUseColorSpecular;

		iGpuProgram* mpSimpleProgram;
		iGpuProgram* mpAmbientProgram;

		/**
		 * The ambient pass of the shading (cShadingSettings): programs of
		 * hpl1_ambient for what the material has, loaded when first used.
		 * False if they could not be loaded.
		 */
		bool UseShadedAmbient();
		iGpuProgram* mpShadedAmbientProgram;
		bool mbShadedAmbientTried;

		iGpuProgram* mvPrograms[eBaseLightProgram_LastEnum];
	};

};
#endif // HPL_MATERIAL_BASE_LIGHT_H
