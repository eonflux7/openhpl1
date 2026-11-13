// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "graphics/Material_Alpha.h"
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

	cMaterial_Alpha::cMaterial_Alpha(	const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
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

	cMaterial_Alpha::~cMaterial_Alpha()
	{
		if(mpFogProgram) mpProgramManager->Destroy(mpFogProgram);
	}

	iGpuProgram* cMaterial_Alpha::GetProgram(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(mpRenderSettings->mbFogActive)
			return mpFogProgram;
		else
			return nullptr;
	}

	iMaterialProgramSetup* cMaterial_Alpha::GetProgramSetup(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(mpRenderSettings->mbFogActive)
			return &gFogProgramSetup;
		else
			return nullptr;
	}

	bool cMaterial_Alpha::ProgramUsesLight(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return false;
	}

	bool cMaterial_Alpha::ProgramUsesEye(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return false;
	}



	eMaterialAlphaMode cMaterial_Alpha::GetAlphaMode(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return eMaterialAlphaMode_Solid;
	}

	eMaterialBlendMode cMaterial_Alpha::GetBlendMode(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return eMaterialBlendMode_Alpha;
	}

	eMaterialChannelMode cMaterial_Alpha::GetChannelMode(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return eMaterialChannelMode_RGBA;
	}

	iTexture* cMaterial_Alpha::GetTexture(int alUnit,eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(alUnit == 0)
			return mvTexture[eMaterialTexture_Diffuse];

		if(alUnit == 1 && mpRenderSettings->mbFogActive)
		{
			return mpRenderer3D->GetFogAlphaTexture();
		}

		return nullptr;
	}

	bool cMaterial_Alpha::UsesType(eMaterialRenderType aType)
	{
		if(aType == eMaterialRenderType_Diffuse) return true;
		return false;
	}

	tTextureTypeList cMaterial_Alpha::GetTextureTypes()
	{
		tTextureTypeList vTypes;
		vTypes.push_back(cTextureType("",eMaterialTexture_Diffuse));
		vTypes.push_back(cTextureType("_ref",eMaterialTexture_Refraction));
		vTypes.push_back(cTextureType("_spec",eMaterialTexture_Specular));

		return vTypes;
	}
}
