// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "graphics/Material_Additive.h"
#include "graphics/Renderer3D.h"
#include "scene/Light.h"
#include "scene/Camera.h"
#include "resources/GpuProgramManager.h"
#include "resources/TextureManager.h"
#include "graphics/GPUProgram.h"
#include "math/Math.h"


namespace hpl {

	// VERTEX PRORGAM SETUP

	class cFogProgramSetup : public iMaterialProgramSetup
	{
	public:
		void Setup(iGpuProgram *apProgram,cRenderSettings* apRenderSettings) override
		{
			apProgram->SetFloat(eProgramParam::fogStart,apRenderSettings->mfFogStart);
			apProgram->SetFloat(eProgramParam::fogEnd,apRenderSettings->mfFogEnd);
		}
	};

	static cFogProgramSetup gFogProgramSetup;

	cMaterial_Additive::cMaterial_Additive(	const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
		cImageManager* apImageManager, cTextureManager *apTextureManager,
		cGpuProgramManager* apProgramManager,
		eMaterialPicture aPicture, cRenderer3D *apRenderer3D)
		: iMaterial(asName,apLowLevelGraphics,apImageManager,apTextureManager,apProgramManager,
					aPicture,apRenderer3D)
	{
		mbIsTransparent = true;
		mbIsGlowing= false;
		mbUsesLights = false;


		mpFogProgram = mpProgramManager->CreateProgram("Fog_Trans");
	}

	cMaterial_Additive::~cMaterial_Additive()
	{
		if(mpFogProgram) mpProgramManager->Destroy(mpFogProgram);
	}

	iGpuProgram* cMaterial_Additive::GetProgram(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(mpRenderSettings->mbFogActive)
			return mpFogProgram;
		else
			return nullptr;
	}

	iMaterialProgramSetup* cMaterial_Additive::GetProgramSetup(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(mpRenderSettings->mbFogActive)
			return &gFogProgramSetup;
		else
			return nullptr;
	}


	bool cMaterial_Additive::ProgramUsesLight(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return false;
	}

	bool cMaterial_Additive::ProgramUsesEye(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return false;
	}



	eMaterialAlphaMode cMaterial_Additive::GetAlphaMode(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return eMaterialAlphaMode_Solid;
	}

	eMaterialBlendMode cMaterial_Additive::GetBlendMode(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return eMaterialBlendMode_Add;
	}

	eMaterialChannelMode cMaterial_Additive::GetChannelMode(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return eMaterialChannelMode_RGBA;
	}

	iTexture* cMaterial_Additive::GetTexture(int alUnit,eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(alUnit == 0)
			return mvTexture[eMaterialTexture_Diffuse];

		if(alUnit == 1 && mpRenderSettings->mbFogActive)
		{
			return mpRenderer3D->GetFogAddTexture();
		}

		return nullptr;
	}

	bool cMaterial_Additive::UsesType(eMaterialRenderType aType)
	{
		if(aType == eMaterialRenderType_Diffuse) return true;
		return false;
	}

	tTextureTypeList cMaterial_Additive::GetTextureTypes()
	{
		tTextureTypeList vTypes;
		vTypes.push_back(cTextureType("",eMaterialTexture_Diffuse));
		vTypes.push_back(cTextureType("_ref",eMaterialTexture_Refraction));
		vTypes.push_back(cTextureType("_spec",eMaterialTexture_Specular));

		return vTypes;
	}
}
