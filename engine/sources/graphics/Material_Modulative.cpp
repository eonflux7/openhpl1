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
#include "graphics/Material_Modulative.h"
#include "graphics/Renderer3D.h"
#include "scene/Light.h"
#include "scene/Camera.h"
#include "resources/GpuProgramManager.h"
#include "resources/TextureManager.h"
#include "graphics/GPUProgram.h"
#include "math/Math.h"


namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// VERTEX PRORGAM SETUP
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	class cFogProgramSetup : public iMaterialProgramSetup
	{
	public:
		void Setup(iGpuProgram *apProgram,cRenderSettings* apRenderSettings) override
		{
			apProgram->SetFloat("fogStart",apRenderSettings->mfFogStart);
			apProgram->SetFloat("fogEnd",apRenderSettings->mfFogEnd);
		}
	};

	static cFogProgramSetup gFogProgramSetup;

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cMaterial_Modulative::cMaterial_Modulative(	const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
		cImageManager* apImageManager, cTextureManager *apTextureManager,
		cGpuProgramManager* apProgramManager,
		eMaterialPicture aPicture, cRenderer3D *apRenderer3D)
		: iMaterial(asName,apLowLevelGraphics,apImageManager,apTextureManager,apProgramManager,
					aPicture,apRenderer3D)
	{
		mbIsTransperant = true;
		mbIsGlowing= false;
		mbUsesLights = false;

		mpFogVtxProg = mpProgramManager->CreateProgram("Fog_Trans_vp.cg","main",eGpuProgramType_Vertex);

		mpFogFragProg = mpProgramManager->CreateProgram("Fog_Trans_Mod_fp.cg","main",eGpuProgramType_Fragment);
	}

	//-----------------------------------------------------------------------

	cMaterial_Modulative::~cMaterial_Modulative()
	{
		if(mpFogVtxProg) mpProgramManager->Destroy(mpFogVtxProg);
		if(mpFogFragProg) mpProgramManager->Destroy(mpFogFragProg);
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	iGpuProgram* cMaterial_Modulative::GetVertexProgram(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(mpRenderSettings->mbFogActive)
			return mpFogVtxProg;
		else
			return nullptr;
	}

	iMaterialProgramSetup* cMaterial_Modulative::GetVertexProgramSetup(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(mpRenderSettings->mbFogActive)
			return &gFogProgramSetup;
		else
			return nullptr;
	}

	bool cMaterial_Modulative::VertexProgramUsesLight(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return false;
	}

	bool cMaterial_Modulative::VertexProgramUsesEye(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return false;
	}

	iGpuProgram* cMaterial_Modulative::GetFragmentProgram(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(mpRenderSettings->mbFogActive)
			return mpFogFragProg;
		else
			return nullptr;
	}

	eMaterialAlphaMode cMaterial_Modulative::GetAlphaMode(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return eMaterialAlphaMode_Solid;
	}

	eMaterialBlendMode cMaterial_Modulative::GetBlendMode(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return eMaterialBlendMode_Mul;
	}

	eMaterialChannelMode cMaterial_Modulative::GetChannelMode(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return eMaterialChannelMode_RGBA;
	}

	//-----------------------------------------------------------------------

	iTexture* cMaterial_Modulative::GetTexture(int alUnit,eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(mpRenderSettings->mbFogActive)
		{
			if(alUnit == 0)
				return mvTexture[eMaterialTexture_Diffuse];
			else if(alUnit == 1)
				return mpRenderer3D->GetFogAddTexture();
		}
		else
		{
			if(alUnit == 0)
				return mvTexture[eMaterialTexture_Diffuse];
		}

		return nullptr;
	}

	eMaterialBlendMode cMaterial_Modulative::GetTextureBlend(int alUnit,eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return eMaterialBlendMode_Mul;
	}

	//-----------------------------------------------------------------------

	bool cMaterial_Modulative::UsesType(eMaterialRenderType aType)
	{
		if(aType == eMaterialRenderType_Diffuse) return true;
		return false;
	}

	//-----------------------------------------------------------------------

	tTextureTypeList cMaterial_Modulative::GetTextureTypes()
	{
		tTextureTypeList vTypes;
		vTypes.push_back(cTextureType("",eMaterialTexture_Diffuse));
		vTypes.push_back(cTextureType("_ref",eMaterialTexture_Refraction));
		vTypes.push_back(cTextureType("_spec",eMaterialTexture_Specular));

		return vTypes;
	}

	//-----------------------------------------------------------------------
}
