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
#ifndef HPL_MATERIAL_ALPHA_H
#define HPL_MATERIAL_ALPHA_H

#include "graphics/Material.h"

namespace hpl {

	class cMaterial_Alpha : public iMaterial
	{
	public:
		cMaterial_Alpha(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cRenderer2D* apRenderer, cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D);

		~cMaterial_Alpha() override;

		tTextureTypeList GetTextureTypes() override;

		bool UsesType(eMaterialRenderType aType) override;

		iGpuProgram* GetVertexProgram(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		iMaterialProgramSetup* GetVertexProgramSetup(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		bool VertexProgramUsesLight(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		bool VertexProgramUsesEye(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;

		iGpuProgram* GetFragmentProgram(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;

		eMaterialAlphaMode GetAlphaMode(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		eMaterialBlendMode GetBlendMode(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		eMaterialChannelMode GetChannelMode(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;

		iTexture* GetTexture(int alUnit,eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		eMaterialBlendMode GetTextureBlend(int alUnit,eMaterialRenderType aType, int alPass, iLight3D *apLight) override;

		int GetNumOfPasses(eMaterialRenderType aType, iLight3D *apLight) override{ return 1;}

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
	private:
		iGpuProgram *mpFogVtxProg;
		iGpuProgram *mpFogFragProg;
	};

	class cMaterialType_Alpha : public iMaterialType
	{
	public:
		bool IsCorrect(tString asName) override{
			return cString::ToLowerCase(asName)=="alpha";
		}

		iMaterial* Create(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cRenderer2D* apRenderer, cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D) override
		{
			return hplNew( cMaterial_Alpha, (asName,apLowLevelGraphics,
				apImageManager,apTextureManager,apRenderer,
				apProgramManager,aPicture,apRenderer3D) );
		}
	};

};
#endif // HPL_MATERIAL_ALPHA_H
