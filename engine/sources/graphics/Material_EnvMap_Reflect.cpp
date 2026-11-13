// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "graphics/Material_EnvMap_Reflect.h"
#include "graphics/Renderer3D.h"
#include "scene/Light.h"
#include "scene/Camera.h"
#include "resources/GpuProgramManager.h"
#include "resources/TextureManager.h"
#include "graphics/GPUProgram.h"
#include "math/Math.h"
#include "system/LowLevelSystem.h"
#include "scene/Camera3D.h"


namespace hpl {

	// MATERIAL SETUP

	void cEnvMapReflect_SetUp::Setup(iGpuProgram *apProgram,cRenderSettings* apRenderSettings)
	{
		apProgram->SetVec3f(eProgramParam::eyeWorldPos,apRenderSettings->mpCamera->GetPosition());
	}

	void cEnvMapReflect_SetUp::SetupMatrix(cMatrixf *apModelMatrix, cRenderSettings* apRenderSettings)
	{
		//Put here so it is updated with every matrix, just aswell...
		if(apModelMatrix)
			apRenderSettings->mpProgram->SetMatrixf(eProgramParam::objectWorldMatrix,*apModelMatrix);
		else
		{
			apRenderSettings->mpProgram->SetMatrixf(eProgramParam::objectWorldMatrix,cMatrixf::Identity);
		}
	}

	static cEnvMapReflect_SetUp gEnvMaterialSetup;

	cMaterial_EnvMap_Reflect::cMaterial_EnvMap_Reflect(	const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
		cImageManager* apImageManager, cTextureManager *apTextureManager,
		cGpuProgramManager* apProgramManager,
		eMaterialPicture aPicture, cRenderer3D *apRenderer3D)
		: iMaterial(asName,apLowLevelGraphics,apImageManager,apTextureManager,apProgramManager,
		aPicture,apRenderer3D)
	{
		mbIsTransparent = false;
		mbIsGlowing= false;
		mbUsesLights = false;

		//Load the Z pass vertex program
		iGpuProgram *pVtxProg = mpProgramManager->CreateProgram("Diffuse_Color");
		SetProgram(pVtxProg,0);

		//Load the Z pass vertex program
		pVtxProg = mpProgramManager->CreateProgram("Diffuse_EnvMap_Reflect");
		SetProgram(pVtxProg,1);
	}

	cMaterial_EnvMap_Reflect::~cMaterial_EnvMap_Reflect()
	{
	}

	iGpuProgram* cMaterial_EnvMap_Reflect::GetProgram(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(aType == eMaterialRenderType_Z)
			return mpProgram[0];
		else
			return mpProgram[1];
	}

	bool cMaterial_EnvMap_Reflect::ProgramUsesLight(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return false;
	}

	bool cMaterial_EnvMap_Reflect::ProgramUsesEye(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return false;
	}

	iMaterialProgramSetup * cMaterial_EnvMap_Reflect::GetProgramSetup(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(aType == eMaterialRenderType_Diffuse) return &gEnvMaterialSetup;

		return nullptr;
	}

	// Both passes are drawn by the vertex program's own fragment shader
	// (hpl1_Diffuse_EnvMap_Reflect.fragment for the diffuse one)


	eMaterialAlphaMode cMaterial_EnvMap_Reflect::GetAlphaMode(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return eMaterialAlphaMode_Solid;
	}

	eMaterialBlendMode cMaterial_EnvMap_Reflect::GetBlendMode(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(aType == eMaterialRenderType_Z) return eMaterialBlendMode_None;

		return eMaterialBlendMode_Replace;
	}

	eMaterialChannelMode cMaterial_EnvMap_Reflect::GetChannelMode(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(aType == eMaterialRenderType_Z) return eMaterialChannelMode_Z;
		return eMaterialChannelMode_RGBA;
	}

	iTexture* cMaterial_EnvMap_Reflect::GetTexture(int alUnit,eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(aType == eMaterialRenderType_Z) return nullptr;

		if(alUnit == 0)
			return mvTexture[eMaterialTexture_Diffuse];
		if(alUnit == 1)
			return mvTexture[eMaterialTexture_CubeMap];

		return nullptr;
	}

	bool cMaterial_EnvMap_Reflect::UsesType(eMaterialRenderType aType)
	{
		if(aType == eMaterialRenderType_Diffuse || aType == eMaterialRenderType_Z) return true;
		return false;
	}

	tTextureTypeList cMaterial_EnvMap_Reflect::GetTextureTypes()
	{
		tTextureTypeList vTypes;
		vTypes.push_back(cTextureType("",eMaterialTexture_Diffuse));
		vTypes.push_back(cTextureType("cube",eMaterialTexture_CubeMap));

		return vTypes;
	}
}
