// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "graphics/Material_Water.h"
#include "graphics/Renderer3D.h"
#include "scene/Light.h"
#include "scene/Camera.h"
#include "resources/GpuProgramManager.h"
#include "resources/TextureManager.h"
#include "graphics/GPUProgram.h"
#include "math/Math.h"


namespace hpl {

	// VERTEX PRORGAM SETUP

	class cFogWaterProgramSetup : public iMaterialProgramSetup
	{
	public:
		void Setup(iGpuProgram *apProgram,cRenderSettings* apRenderSettings) override
		{
			apProgram->SetFloat(eProgramParam::fogStart,apRenderSettings->mfFogStart);
			apProgram->SetFloat(eProgramParam::fogEnd,apRenderSettings->mfFogEnd);
		}
		float mfTime;
	};

	static cFogWaterProgramSetup gFogWaterProgramSetup;

	class cWaterProgramSetup : public iMaterialProgramSetup
	{
	public:
		void Setup(iGpuProgram *apProgram,cRenderSettings* apRenderSettings) override
		{
			apProgram->SetFloat(eProgramParam::timeCount,mfTime);
		}

		float mfTime;
	};

	static cWaterProgramSetup gWaterProgramSetup;

	cMaterial_Water::cMaterial_Water(	const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
		cImageManager* apImageManager, cTextureManager *apTextureManager,
		cGpuProgramManager* apProgramManager,
		eMaterialPicture aPicture, cRenderer3D *apRenderer3D)
		: iMaterial(asName,apLowLevelGraphics,apImageManager,apTextureManager,apProgramManager,
					aPicture,apRenderer3D)
	{
		mbIsTransparent = true;
		mbIsGlowing= false;
		mbUsesLights = false;


		mpFogProgram = mpProgramManager->CreateProgram("Water_Fog");
		mpRefractProgram = mpProgramManager->CreateProgram("refract_water");

		iGpuProgram *pVtxProg = mpProgramManager->CreateProgram("Water_Diffuse");
		SetProgram(pVtxProg,0);

		mfTime = 0;
	}

	cMaterial_Water::~cMaterial_Water()
	{
		if(mpFogProgram) mpProgramManager->Destroy(mpFogProgram);
		if(mpRefractProgram) mpProgramManager->Destroy(mpRefractProgram);
	}

	void cMaterial_Water::Update(float afTimeStep)
	{
		mfTime += afTimeStep;
		gWaterProgramSetup.mfTime = mfTime;
		gFogWaterProgramSetup.mfTime = mfTime;
	}

	iGpuProgram* cMaterial_Water::GetProgram(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(mpRenderSettings->mbFogActive)
			return mpFogProgram;
		else
			return mpProgram[0];
	}

	iMaterialProgramSetup* cMaterial_Water::GetProgramSetup(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(mpRenderSettings->mbFogActive)
			return &gFogWaterProgramSetup;
		else
			return &gWaterProgramSetup;
	}

	bool cMaterial_Water::ProgramUsesLight(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return false;
	}

	bool cMaterial_Water::ProgramUsesEye(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return false;
	}



	eMaterialAlphaMode cMaterial_Water::GetAlphaMode(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return eMaterialAlphaMode_Solid;
	}

	eMaterialBlendMode cMaterial_Water::GetBlendMode(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return eMaterialBlendMode_Mul;//Add;
	}

	eMaterialChannelMode cMaterial_Water::GetChannelMode(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return eMaterialChannelMode_RGBA;
	}

	iTexture* cMaterial_Water::GetTexture(int alUnit,eMaterialRenderType aType, int alPass, iLight3D *apLight)
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

	bool cMaterial_Water::UsesType(eMaterialRenderType aType)
	{
		if(aType == eMaterialRenderType_Diffuse) return true;
		return false;
	}

	tTextureTypeList cMaterial_Water::GetTextureTypes()
	{
		tTextureTypeList vTypes;
		vTypes.push_back(cTextureType("",eMaterialTexture_Diffuse));
		vTypes.push_back(cTextureType("",eMaterialTexture_Specular));

		if(mpRefractProgram)
			vTypes.push_back(cTextureType("",eMaterialTexture_Refraction));

		return vTypes;
	}
}
