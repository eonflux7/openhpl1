/*
 * Copyright (C) 2006-2010 - Frictional Games
 *
 * This file is part of HPL1 Engine.
 *
 * HPL1 Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HPL1 Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HPL1 Engine.  If not, see <http://www.gnu.org/licenses/>.
 */
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
		iMaterial_BaseLight(const tString& asLightVertexProgram,
							const tString& asLightFragmentProgram,

			const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D);

		~iMaterial_BaseLight() override;

		tTextureTypeList GetTextureTypes() override;

		bool UsesType(eMaterialRenderType aType) override;

		iGpuProgram* GetVertexProgram(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		bool VertexProgramUsesLight(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		bool VertexProgramUsesEye(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		iMaterialProgramSetup * GetVertexProgramSetup(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;

		iGpuProgram* GetFragmentProgram(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		iMaterialProgramSetup * GetFragmentProgramSetup(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;

		eMaterialAlphaMode GetAlphaMode(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		eMaterialBlendMode GetBlendMode(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		eMaterialChannelMode GetChannelMode(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;

		iTexture* GetTexture(int alUnit,eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		eMaterialBlendMode GetTextureBlend(int alUnit,eMaterialRenderType aType, int alPass, iLight3D *apLight) override;

		int GetNumOfPasses(eMaterialRenderType aType, iLight3D *apLight) override;

		//////////////////////////////////////////////////////////////////
		// Old and worthless stuff, only used by 2D renderer
		void Compile() override{}
		bool StartRendering(eMaterialRenderType aType,iCamera* apCam,iLight *pLight) override{return false;}
		void EndRendering(eMaterialRenderType aType) override{}
		tVtxBatchFlag GetBatchFlags(eMaterialRenderType aType) override{return 0;}
		bool NextPass(eMaterialRenderType aType) override{return false;}
		bool HasMultiplePasses(eMaterialRenderType aType) override{return false;}
		eMaterialType GetType(eMaterialRenderType aType) override{ return eMaterialType_Diffuse;}
		void EditVertexes(eMaterialRenderType aType, iCamera* apCam, iLight *pLight,
			tVertexVec *apVtxVec,cVector3f *apTransform,unsigned int alIndexAdd) override{}

	protected:
		//properties to set
		bool mbUseSpecular;
		bool mbUseNormalMap;
		bool mbUseColorSpecular;

		iGpuProgram* mpSimpleFP;
		iGpuProgram* mpAmbientFP;

		/**
		 * The ambient pass of the improved shading (cShadingSettings):
		 * programs of hpl1_ambient for what the material has, loaded when
		 * the option is first on. False if they could not be loaded.
		 */
		bool UseShadedAmbient();
		iGpuProgram* mpShadedAmbientVP;
		iGpuProgram* mpShadedAmbientFP;
		bool mbShadedAmbientTried;

		iGpuProgram* mvVtxPrograms[eBaseLightProgram_LastEnum];
		iGpuProgram* mvFragPrograms[eBaseLightProgram_LastEnum];
	};

};
#endif // HPL_MATERIAL_BASE_LIGHT_H
