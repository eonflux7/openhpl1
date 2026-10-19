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
#include "graphics/Material_BaseLight.h"
#include "scene/Light.h"
#include "scene/Camera.h"
#include "resources/GpuProgramManager.h"
#include "resources/TextureManager.h"
#include "graphics/GPUProgram.h"
#include "math/Math.h"
#include "system/String.h"
#include "scene/Light3DSpot.h"
#include "graphics/Renderer3D.h"
#include "scene/PortalContainer.h"

namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// FRAGMENT PRORGAM SETUP
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	class cAmbProgramSetup : public iMaterialProgramSetup
	{
	public:
		void Setup(iGpuProgram *apProgram,cRenderSettings* apRenderSettings) override
		{
			if(apRenderSettings->mpSector)
				apProgram->SetColor3f(eProgramParam::ambientColor,apRenderSettings->mAmbientColor * apRenderSettings->mpSector->GetAmbientColor());
			else
				apProgram->SetColor3f(eProgramParam::ambientColor,apRenderSettings->mAmbientColor);
		}
	};

	static cAmbProgramSetup gAmbProgramSetup;

	//-----------------------------------------------------------------------

	// The shaded ambient pass needs the surface's place in the world
	class cShadedAmbientSetup : public iMaterialProgramSetup
	{
	public:
		void Setup(iGpuProgram *apProgram,cRenderSettings* apRenderSettings) override
		{
			gAmbProgramSetup.Setup(apProgram, apRenderSettings);
		}

		void SetupMatrix(cMatrixf *apModelMatrix, cRenderSettings* apRenderSettings) override
		{
			apRenderSettings->mpProgram->SetMatrixf(eProgramParam::objectWorldMatrix,
												apModelMatrix ? *apModelMatrix : cMatrixf::Identity);
		}
	};

	static cShadedAmbientSetup gShadedAmbientSetup;

	//-----------------------------------------------------------------------


	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	iMaterial_BaseLight::iMaterial_BaseLight(const tString& asLightDefines,
		const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
		cImageManager* apImageManager, cTextureManager *apTextureManager,
		cGpuProgramManager* apProgramManager,
		eMaterialPicture aPicture, cRenderer3D *apRenderer3D)
		: iMaterial(asName,apLowLevelGraphics,apImageManager,apTextureManager,apProgramManager,
					aPicture,apRenderer3D)
	{
		mbIsTransparent = false;
		mbIsGlowing= false;
		mbUsesLights = true;
		mbUseColorSpecular = false;

		for(int i=0; i<eBaseLightProgram_LastEnum; i++) {
			mvPrograms[i] =nullptr;
		}

		mvPrograms[eBaseLightProgram_Point1] = mpProgramManager->CreateProgram("light", asLightDefines);
		mvPrograms[eBaseLightProgram_Spot1] = mpProgramManager->CreateProgram("light", asLightDefines + " SPOT");

		///////////////////////////////////////////
		//More fragment programs
		mpSimpleProgram = mpProgramManager->CreateProgram("Diffuse_Color");
		mpAmbientProgram = mpProgramManager->CreateProgram("Ambient_Color", "", "Diffuse_Color");


		mbUseSpecular = false;
		mbUseNormalMap = false;

		mpShadedAmbientProgram = nullptr;
		mbShadedAmbientTried = false;
	}

	//-----------------------------------------------------------------------

	iMaterial_BaseLight::~iMaterial_BaseLight()
	{
		for(int i=0; i<eBaseLightProgram_LastEnum; i++)
		{
			if(mvPrograms[i])	mpProgramManager->Destroy(mvPrograms[i]);
		}

		if(mpSimpleProgram) mpProgramManager->Destroy(mpSimpleProgram);
		if(mpAmbientProgram) mpProgramManager->Destroy(mpAmbientProgram);
		if(mpShadedAmbientProgram) mpProgramManager->Destroy(mpShadedAmbientProgram);
	}

	//-----------------------------------------------------------------------

	bool iMaterial_BaseLight::UseShadedAmbient()
	{
		// Not in the constructor: what the material has is set by the
		// constructors of the classes that inherit this one.
		if(mbShadedAmbientTried==false)
		{
			mbShadedAmbientTried = true;

			tString defines;
			if(mbUseNormalMap) defines += "NORMAL_MAP ";
			if(mbUseSpecular) defines += mbUseColorSpecular ? "SPECULAR SPECULAR_MAP" : "SPECULAR";
			mpShadedAmbientProgram = mpProgramManager->CreateProgram("ambient", defines);

		}

		return mpShadedAmbientProgram != nullptr;
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------



	//------------------------------------------------------------------------------------

	bool iMaterial_BaseLight::ProgramUsesLight(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(aType == eMaterialRenderType_Light) return true;
		return false;
	}

	//------------------------------------------------------------------------------------

	bool iMaterial_BaseLight::ProgramUsesEye(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(aType == eMaterialRenderType_Light && mbUseSpecular) return true;
		return false;
	}

	//------------------------------------------------------------------------------------



	//------------------------------------------------------------------------------------

	iGpuProgram* iMaterial_BaseLight::GetProgram(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(aType == eMaterialRenderType_Light)
		{
			eBaseLightProgram program = apLight->GetLightType()==eLight3DType_Spot ?
				eBaseLightProgram_Spot1 : eBaseLightProgram_Point1;

			return mvPrograms[program];
		}
		else if(aType == eMaterialRenderType_Diffuse)
		{
			return mpSimpleProgram;
		}
		else if(aType == eMaterialRenderType_Z)
		{
			return UseShadedAmbient() ? mpShadedAmbientProgram : mpAmbientProgram;
		}
		return nullptr;
	}


	iMaterialProgramSetup * iMaterial_BaseLight::GetProgramSetup(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(aType == eMaterialRenderType_Z)
		{
			return UseShadedAmbient() ? static_cast<iMaterialProgramSetup*>(&gShadedAmbientSetup) : &gAmbProgramSetup;
		}

		return nullptr;
	}

	//------------------------------------------------------------------------------------

	eMaterialAlphaMode iMaterial_BaseLight::GetAlphaMode(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(aType == eMaterialRenderType_Z && mbHasAlpha) return eMaterialAlphaMode_Trans;

		return eMaterialAlphaMode_Solid;
	}

	//------------------------------------------------------------------------------------

	eMaterialBlendMode iMaterial_BaseLight::GetBlendMode(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(aType == eMaterialRenderType_Z) return eMaterialBlendMode_Replace;

		return eMaterialBlendMode_Add;
	}

	//------------------------------------------------------------------------------------

	eMaterialChannelMode iMaterial_BaseLight::GetChannelMode(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return eMaterialChannelMode_RGBA;
	}

	//-----------------------------------------------------------------------

	iTexture* iMaterial_BaseLight::GetTexture(int alUnit,eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(aType == eMaterialRenderType_Z)
		{
			if(alUnit==0) return mvTexture[eMaterialTexture_Diffuse];
			if(UseShadedAmbient())
			{
				if(alUnit==1 && mbUseNormalMap) return mvTexture[eMaterialTexture_NMap];
				if(alUnit==6 && mbUseColorSpecular) return mvTexture[eMaterialTexture_Specular];
			}
			return nullptr;
		}
		else if(aType == eMaterialRenderType_Diffuse)
		{
			if(alUnit==0) return mvTexture[eMaterialTexture_Illumination];
			return nullptr;
		}
		else if(aType == eMaterialRenderType_Light)
		{
			switch(alUnit)
			{
				//Diffuse texture
			case 0: return mvTexture[eMaterialTexture_Diffuse];

				//Normalmap
			case 1: if(mbUseNormalMap) return mvTexture[eMaterialTexture_NMap];break;

				//Falloff map
			case 3: return apLight->GetFalloffMap();

				//Spotlight texture
			case 4: if(apLight->GetLightType() == eLight3DType_Spot)
					{
						cLight3DSpot *pSpotLight = static_cast<cLight3DSpot*>(apLight);
						return pSpotLight->GetTexture();
					};
				break;
				//Color specular
			case 6:	if(mbUseColorSpecular)
					{
						return mvTexture[eMaterialTexture_Specular];
					}
					break;
			}
		}
		return nullptr;
	}

	//-----------------------------------------------------------------------

	//-----------------------------------------------------------------------

	int iMaterial_BaseLight::GetNumOfPasses(eMaterialRenderType aType, iLight3D *apLight)
	{
		return 1;
	}

	//-----------------------------------------------------------------------

	bool iMaterial_BaseLight::UsesType(eMaterialRenderType aType)
	{
		if(aType == eMaterialRenderType_Diffuse)
		{
			if(mvTexture[eMaterialTexture_Illumination])
				return true;
			else
				return false;
		}
		return true;
	}

	//-----------------------------------------------------------------------

	tTextureTypeList iMaterial_BaseLight::GetTextureTypes()
	{
		tTextureTypeList vTypes;
		vTypes.push_back(cTextureType("",eMaterialTexture_Diffuse));
		if(mbUseNormalMap)
			vTypes.push_back(cTextureType("_bump",eMaterialTexture_NMap));

		if(mbUseColorSpecular)
			vTypes.push_back(cTextureType("_spec",eMaterialTexture_Specular));

		vTypes.push_back(cTextureType("_illum",eMaterialTexture_Illumination));

		return vTypes;
	}

	//-----------------------------------------------------------------------
}
