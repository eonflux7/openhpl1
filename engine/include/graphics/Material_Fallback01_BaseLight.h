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
#ifndef HPL_MATERIAL_FALLBACK01_BASE_LIGHT_H
#define HPL_MATERIAL_FALLBACK01_BASE_LIGHT_H

#include <vector>
#include "graphics/Material.h"
#include "scene/Light3D.h"

#include "graphics/Material_BaseLight.h"

namespace hpl {

	//---------------------------------------------------------------

	class iMaterial_Fallback01_BaseLight : public iMaterial
	{
	public:
		iMaterial_Fallback01_BaseLight(bool abNormalMap, bool abSpecular,
			const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cRenderer2D* apRenderer, cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D);

		~iMaterial_Fallback01_BaseLight() override;

		tTextureTypeList GetTextureTypes() override;

		bool UsesType(eMaterialRenderType aType) override;

		iGpuProgram* GetVertexProgram(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		bool VertexProgramUsesLight(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;
		bool VertexProgramUsesEye(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;

		iGpuProgram* GetFragmentProgram(eMaterialRenderType aType, int alPass, iLight3D *apLight) override;

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
		iTexture *mpNormalizationMap;
		iTexture *mpSpotNegativeRejectMap;

		//properties to set
		bool mbUseSpecular;
		bool mbUseNormalMap;

		bool mbUsesTwoPassSpot;

		iTexture *mpAttenuationMap;

		iGpuProgram* mvVtxPrograms[eBaseLightProgram_LastEnum];
		iGpuProgram* mvFragPrograms[eBaseLightProgram_LastEnum];
	};

	//---------------------------------------------------------------

	class cGLState_Diffuse : public iGLStateProgram
	{
	public:
		cGLState_Diffuse();

		void Bind() override;
		void UnBind() override;
	private:
		void InitData() override{}
	};

	//---------------------------------------------------------------

	class cGLState_ATIDiffuse : public iGLStateProgram
	{
	public:
		cGLState_ATIDiffuse();
		~cGLState_ATIDiffuse() override;

		void Bind() override;
		void UnBind() override;
	private:
		void InitData() override;

		int mlBind;
	};

	//---------------------------------------------------------------


	class cGLState_Bump : public iGLStateProgram
	{
	public:
		cGLState_Bump();

		void Bind() override;
		void UnBind() override;
	private:
		void InitData() override{}
	};

	//---------------------------------------------------------------

	class cGLState_ATIBump : public iGLStateProgram
	{
	public:
		cGLState_ATIBump();
		~cGLState_ATIBump() override;

		void Bind() override;
		void UnBind() override;
	private:
		void InitData() override;

		int mlBind;
	};

	//---------------------------------------------------------------

	class cGLState_Spot : public iGLStateProgram
	{
	public:
		cGLState_Spot();

		void Bind() override;
		void UnBind() override;
	private:
		void InitData() override{}
	};

	//---------------------------------------------------------------

	class cGLState_ATISpot : public iGLStateProgram
	{
	public:
		cGLState_ATISpot();
		~cGLState_ATISpot() override;

		void Bind() override;
		void UnBind() override;
	private:
		void InitData() override;

		int mlBind;
	};

	//---------------------------------------------------------------

	///////////////////////////////////////////
	// Diffuse
	///////////////////////////////////////////

	class cMaterial_Fallback01_Diffuse : public iMaterial_Fallback01_BaseLight
	{
	public:
		cMaterial_Fallback01_Diffuse(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cRenderer2D* apRenderer, cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D)
		: iMaterial_Fallback01_BaseLight(false, false,
				asName,apLowLevelGraphics,apImageManager,apTextureManager,apRenderer,apProgramManager,
				aPicture,apRenderer3D)
		{
		}
	};

	///////////////////////////////////////////
	// Bump
	///////////////////////////////////////////

	class cMaterial_Fallback01_Bump : public iMaterial_Fallback01_BaseLight
	{
	public:
		cMaterial_Fallback01_Bump(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cRenderer2D* apRenderer, cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D)
			: iMaterial_Fallback01_BaseLight(true, false,
					asName,apLowLevelGraphics,apImageManager,apTextureManager,apRenderer,apProgramManager,
					aPicture,apRenderer3D)
		{
		}
	};


	//---------------------------------------------------------------

};
#endif // HPL_MATERIAL_FALLBACK01_BASE_LIGHT_H
