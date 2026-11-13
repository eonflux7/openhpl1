// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "graphics/Material_Translucent.h"
#include "graphics/Renderer3D.h"
#include "resources/GpuProgramManager.h"
#include "graphics/GPUProgram.h"

namespace hpl {

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

	cMaterial_Translucent::cMaterial_Translucent(const cTranslucentMaterialMode &aMode, const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
		cImageManager* apImageManager, cTextureManager *apTextureManager,
		cGpuProgramManager* apProgramManager,
		eMaterialPicture aPicture, cRenderer3D *apRenderer3D)
		: iMaterial(asName,apLowLevelGraphics,apImageManager,apTextureManager,apProgramManager,
					aPicture,apRenderer3D), mMode(aMode)
	{
		mbIsTransparent = true;
		mbIsGlowing= false;
		mbUsesLights = false;

		mpFogProgram = mpProgramManager->CreateProgram("Fog_Trans", mMode.msFogDefines);
	}

	cMaterial_Translucent::~cMaterial_Translucent()
	{
		if(mpFogProgram) mpProgramManager->Destroy(mpFogProgram);
	}

	iGpuProgram* cMaterial_Translucent::GetProgram(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return mpRenderSettings->mbFogActive ? mpFogProgram : nullptr;
	}

	iMaterialProgramSetup* cMaterial_Translucent::GetProgramSetup(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return mpRenderSettings->mbFogActive ? &gFogProgramSetup : nullptr;
	}

	iTexture* cMaterial_Translucent::GetTexture(int alUnit,eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(alUnit == 0)
			return mvTexture[eMaterialTexture_Diffuse];

		if(alUnit == 1 && mpRenderSettings->mbFogActive)
			return mMode.mbAlphaFog ? mpRenderer3D->GetFogAlphaTexture() : mpRenderer3D->GetFogAddTexture();

		return nullptr;
	}

	tTextureTypeList cMaterial_Translucent::GetTextureTypes()
	{
		tTextureTypeList vTypes;
		vTypes.push_back(cTextureType("",eMaterialTexture_Diffuse));
		vTypes.push_back(cTextureType("_ref",eMaterialTexture_Refraction));
		vTypes.push_back(cTextureType("_spec",eMaterialTexture_Specular));

		return vTypes;
	}
}
